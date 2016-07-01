

//
// suppress vs warnings 
//
#if (defined(_WIN32) || defined(WIN32))

#ifndef _CRT_SECURE_NO_DEPRECATE
    #define _CRT_SECURE_NO_DEPRECATE
#endif

/* VC7
4668  : not defined as a macro
4820  : n bytes of padding added
4217  : member template functions cannot be used for copy-assignment 
        or copy-construction
4530  : C++ exception handler used, but unwind semantics are not enabled.  
        <ostream>
4619 : #pragma warning : there is no warning number '4284'
*/
#pragma warning ( disable : 4619 4668 4820 4217 4530  ) 

/* VC8
4514  : unreferenced inline function has been removed 
4530: C++ exception handler used, but unwind semantics are not enabled
*/
#pragma warning ( disable : 4514 )

#pragma warning ( disable : 4244 4255 )


#endif /* WIN32 */
