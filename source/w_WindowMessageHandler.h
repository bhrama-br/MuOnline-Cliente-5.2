#pragma once

namespace util
{
	struct WindowMessageHandler
	{
		// "= 0 {}" e extensao do MSVC; a forma padrao (declaracao pura + definicao
		// fora de linha) compila igual no MSVC e no clang do Emscripten/NDK.
		virtual ~WindowMessageHandler(void) = 0;
		virtual bool HandleWindowMessage( UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result ) = 0;
	};

	inline WindowMessageHandler::~WindowMessageHandler(void) {}
};
