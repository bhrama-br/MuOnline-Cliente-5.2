//////////////////////////////////////////////////////////////////////////
//  - ½Ì±ÛÅæ -
//  
//  
//////////////////////////////////////////////////////////////////////////
#ifndef __SINGLETON_H__
#define __SINGLETON_H__

#include <stdint.h>

/*+++++++++++++++++++++++++++++++++++++
    CLASS.
+++++++++++++++++++++++++++++++++++++*/
template <typename T> 
class Singleton
{
    static T* _Singleton;

public:
    Singleton( void )
    {
        if ( _Singleton==0 )
        {
            // intptr_t, nao int: o deslocamento entre a subobjeto-base e o
            // objeto derivado e aritmetica de PONTEIRO. Em 64 bits (arm64 do
            // Android) int tem 32 bits e truncaria o endereco de `this`,
            // apontando o singleton para lixo. Em 32 bits o resultado e o mesmo
            // de antes.
            intptr_t offset = (intptr_t)(T*)1 - (intptr_t)( Singleton <T>*)(T*)1;
            _Singleton = (T*)((intptr_t)this + offset);
        }
    }
    
    virtual ~Singleton( void ) {  /*assert( _Singleton );*/  _Singleton = 0;  }
    
    static T&   GetSingleton ( void )      {  /*assert( _Singleton );*/  return ( *_Singleton );  }
    static T*   GetSingletonPtr ( void )   {  return ( _Singleton ); } 
	static bool IsInitialized ( void )     { return _Singleton ? true : false; }

	//¿©±â ºÎºÐÀº Á» »ý°¢À» ÇØº¸ÀÚ..
	//new·Î ¸¸µé¾î¼­ ³ÖÀ¸¸é...delete¸¦ ÇØÁà¾ß ÇÏ´Âµ¥...
	//ÇÒ·Á¸é boost·Î ¸¸µé¾îÁø data¸¸ ³Öµµ·Ï ÇÏÀÚ.
	//static void RegisterSingleton ( T* p ) { _Singleton = p; }
};

template <typename T> T* Singleton <T>::_Singleton = 0;

#endif