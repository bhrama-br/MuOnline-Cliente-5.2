package com.roxgaming.mulegacy;

import android.app.Activity;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.system.ErrnoException;
import android.system.Os;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.MotionEvent;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public final class MainActivity extends Activity implements SurfaceHolder.Callback {
    /**
     * A biblioteca NAO e carregada num bloco `static`.
     *
     * Um bloco static roda quando a classe e carregada, ou seja, ANTES de
     * onCreate. Como o codigo de jogo tem construtores globais que ja abrem
     * arquivos ("Data\Local\Mix.bmd"), carregar a .so cedo demais fazia esses
     * construtores rodarem antes de os assets serem extraidos e antes de o
     * diretorio de trabalho existir — e o carregador segfaultava no arquivo
     * ausente.
     *
     * A ordem correta e: extrair assets, publicar a raiz em MU_DATA_ROOT e so
     * entao carregar a .so. Um construtor de prioridade alta no lado nativo le
     * essa variavel e faz o chdir antes de qualquer construtor de C++.
     */
    private static boolean libraryLoaded;

    /** Chamadas SO pela thread de render: o contexto EGL tem afinidade de thread. */
    private native void nativeSurfaceReady(Surface surface, int width, int height);
    private native void nativeSurfaceGone();
    private native void nativeRenderFrame();

    private native void nativeTouchEvent(int action, float x, float y);
    private native void nativeSetDataRoot(String path);

    private RenderThread renderThread;

    /**
     * Laco de render numa thread propria.
     *
     * Antes o desenho vinha do Choreographer, ou seja, da thread de UI, e o
     * contexto EGL era criado em surfaceCreated -- tambem na de UI. Funcionou
     * enquanto a cena era um terreno de teste. Com a subida de cena real (que faz
     * minutos de I/O sincrono e decodificacao de textura) a UI travava e o sistema
     * matava o app: `SIGNALED status=9`, sem tombstone, o que nao se parece com
     * crash e por isso levou um tempo para ser identificado.
     *
     * Contexto GL tem AFINIDADE DE THREAD, entao criacao, carga e desenho tem de
     * viver na MESMA thread -- nao basta mover so o desenho.
     *
     * A thread de UI apenas anota o que aconteceu com a superficie e acorda esta;
     * quem chama qualquer coisa de GL e sempre esta.
     */
    private final class RenderThread extends Thread {
        private Surface surface;
        private int width;
        private int height;
        private boolean bindPendente;     // ha superficie nova para ligar
        private boolean liberarPendente;  // a superficie precisa ser devolvida
        private boolean ligada;           // o nativo detem contexto/superficie
        private boolean encerrar;

        RenderThread() {
            super("MuLegacyRender");
        }

        void superficieDisponivel(Surface novaSurface, int novaLargura, int novaAltura) {
            synchronized (this) {
                surface = novaSurface;
                width = novaLargura;
                height = novaAltura;
                bindPendente = true;
                notifyAll();
            }
        }

        /**
         * Espera a thread de render devolver a superficie, com PRAZO.
         *
         * Esperar e o certo: depois de surfaceDestroyed retornar, o Android pode
         * destruir a superficie, e usa-la apos isso e comportamento indefinido.
         *
         * Mas esperar SEM PRAZO estava errado, e de um jeito que so aparece quando a
         * Activity e recriada durante a carga. A thread de render passa minutos
         * dentro de nativeRenderFrame fazendo I/O e nao ve liberarPendente; a thread
         * de UI ficava ai parada e o Android acusava ANR -- "MU Legacy isn't
         * responding", com o log registrando finishDrawing de 89 s.
         *
         * Com prazo, escolhe-se o menor de dois males: um uso possivelmente breve de
         * superficie morta (que o EGL reporta como EGL_BAD_SURFACE no swap, sem
         * derrubar o processo) em vez de travar a thread de UI, que o sistema pune
         * matando o app. O caso comum -- thread ociosa desenhando quadros curtos --
         * devolve a superficie na primeira volta do laco e nunca chega ao prazo.
         */
        private static final long PRAZO_LIBERACAO_MS = 5000;

        void superficiePerdida() {
            synchronized (this) {
                liberarPendente = true;
                notifyAll();

                final long limite = System.currentTimeMillis() + PRAZO_LIBERACAO_MS;
                while (ligada) {
                    final long restante = limite - System.currentTimeMillis();
                    if (restante <= 0) {
                        // Vale registrar: se isto aparecer, a thread de render ficou
                        // presa numa etapa longa e a superficie foi abandonada sem
                        // ela saber.
                        Log.w("MuLegacy", "superficie nao devolvida em "
                                + PRAZO_LIBERACAO_MS + " ms; seguindo sem esperar");
                        return;
                    }
                    try {
                        wait(restante);
                    } catch (InterruptedException interrompida) {
                        Thread.currentThread().interrupt();
                        return;
                    }
                }
            }
        }

        void encerrarEAguardar() {
            synchronized (this) {
                encerrar = true;
                notifyAll();
            }
            try {
                join(3000);
            } catch (InterruptedException interrompida) {
                Thread.currentThread().interrupt();
            }
        }

        @Override
        public void run() {
            while (true) {
                Surface paraLigar = null;
                int larguraParaLigar = 0;
                int alturaParaLigar = 0;
                boolean liberar = false;

                synchronized (this) {
                    // Dorme enquanto nao houver nada a fazer. Quando a superficie
                    // esta ligada, nao dorme: desenha quadro a quadro.
                    while (!encerrar && !bindPendente && !liberarPendente && !ligada) {
                        try {
                            wait();
                        } catch (InterruptedException interrompida) {
                            Thread.currentThread().interrupt();
                            return;
                        }
                    }

                    if (liberarPendente || (encerrar && ligada)) {
                        liberarPendente = false;
                        liberar = ligada;
                    } else if (encerrar) {
                        return;
                    } else if (bindPendente) {
                        bindPendente = false;
                        paraLigar = surface;
                        larguraParaLigar = width;
                        alturaParaLigar = height;
                    }
                }

                if (liberar) {
                    nativeSurfaceGone();
                    synchronized (this) {
                        ligada = false;
                        notifyAll();   // solta quem espera em superficiePerdida
                    }
                    continue;
                }

                if (paraLigar != null) {
                    nativeSurfaceReady(paraLigar, larguraParaLigar, alturaParaLigar);
                    synchronized (this) {
                        ligada = true;
                    }
                }

                nativeRenderFrame();
            }
        }
    }

    /**
     * Copia os assets empacotados para o armazenamento do app.
     *
     * O codigo de jogo abre arquivos com stdio ("Data\World10\..."), e o
     * AssetManager do Android nao expoe caminho de sistema de arquivos. Extrair
     * uma vez e depois apontar o diretorio de trabalho para ca mantem os
     * carregadores legados intactos.
     */
    private void extractAssets(String relativePath, File targetRoot) throws IOException {
        AssetManager assets = getAssets();
        String[] children = assets.list(relativePath);
        if (children != null && children.length > 0) {
            File dir = new File(targetRoot, relativePath);
            if (!dir.exists() && !dir.mkdirs()) {
                throw new IOException("nao foi possivel criar " + dir);
            }
            for (String child : children) {
                extractAssets(relativePath.isEmpty() ? child : relativePath + "/" + child, targetRoot);
            }
            return;
        }
        File target = new File(targetRoot, relativePath);
        // Ja extraido numa execucao anterior: nao reescreve.
        if (target.exists() && target.length() > 0) return;
        File parent = target.getParentFile();
        if (parent != null && !parent.exists() && !parent.mkdirs()) {
            throw new IOException("nao foi possivel criar " + parent);
        }
        try (InputStream in = assets.open(relativePath);
             OutputStream out = new FileOutputStream(target)) {
            byte[] buffer = new byte[64 * 1024];
            int read;
            while ((read = in.read(buffer)) > 0) out.write(buffer, 0, read);
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // A arvore de dados do cliente tem ~0,94 GB; empacotar tudo no APK nao e
        // viavel, e a cena de login sozinha pede mais de 100 MB. Entao, se o
        // diretorio EXTERNO do app tiver uma pasta Data, ela ganha: e o unico
        // caminho que `adb push` alcanca sem exigir permissao nem root, o que
        // permite testar com o conjunto completo sem inflar o APK.
        //
        // Sem ela, o comportamento e o de antes: extrair do APK o subconjunto de
        // muDataSubsets para o armazenamento interno.
        File dataRoot = null;
        File externo = getExternalFilesDir(null);
        Log.i("MuLegacy", "externo = " + (externo == null ? "null" : externo.getAbsolutePath()));
        if (externo != null) {
            File dataExterno = new File(externo, "Data");
            String[] conteudo = dataExterno.list();
            Log.i("MuLegacy", "Data externo: existe=" + dataExterno.exists()
                    + " dir=" + dataExterno.isDirectory()
                    + " entradas=" + (conteudo == null ? "null" : String.valueOf(conteudo.length)));
            if (conteudo != null && conteudo.length > 0) {
                dataRoot = externo;
                Log.i("MuLegacy", "usando dados externos: " + dataExterno.getAbsolutePath()
                        + " (" + conteudo.length + " entradas)");
            }
        }

        if (dataRoot == null) {
            dataRoot = getFilesDir();
            try {
                extractAssets("Data", dataRoot);
            } catch (IOException error) {
                Log.e("MuLegacy", "falha ao extrair os assets", error);
            }
        }

        if (!libraryLoaded) {
            try {
                // Lida pelo construtor de prioridade 101 em AndroidPlatform.cpp,
                // que faz o chdir antes dos construtores globais do jogo.
                Os.setenv("MU_DATA_ROOT", dataRoot.getAbsolutePath(), true);
            } catch (ErrnoException error) {
                Log.e("MuLegacy", "nao foi possivel publicar MU_DATA_ROOT", error);
            }
            System.loadLibrary("mu_legacy_platform");
            libraryLoaded = true;
        }
        nativeSetDataRoot(dataRoot.getAbsolutePath());

        SurfaceView surfaceView = new SurfaceView(this);
        surfaceView.setFocusable(true);
        surfaceView.setFocusableInTouchMode(true);
        surfaceView.requestFocus();
        surfaceView.setOnTouchListener((view, event) -> {
            nativeTouchEvent(event.getActionMasked(), event.getX(), event.getY());
            return true;
        });
        surfaceView.getHolder().addCallback(this);
        setContentView(surfaceView);
    }

    // Os tres callbacks abaixo rodam na thread de UI e NAO tocam GL: apenas
    // repassam o estado da superficie para a thread de render.

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if (renderThread == null) {
            renderThread = new RenderThread();
            renderThread.start();
        }
        renderThread.superficieDisponivel(holder.getSurface(),
                holder.getSurfaceFrame().width(), holder.getSurfaceFrame().height());
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        if (renderThread != null) {
            renderThread.superficieDisponivel(holder.getSurface(), width, height);
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        // Bloqueia de proposito: depois deste retorno a superficie pode deixar de
        // existir, e a thread de render ainda a estaria usando.
        if (renderThread != null) {
            renderThread.superficiePerdida();
        }
    }

    @Override
    protected void onDestroy() {
        if (renderThread != null) {
            renderThread.encerrarEAguardar();
            renderThread = null;
        }
        super.onDestroy();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        nativeTouchEvent(event.getActionMasked(), event.getX(), event.getY());
        return true;
    }
}
