#ifdef _WIN32
# pragma warning(disable: 4456)
# pragma warning(disable: 4459)
# pragma warning(disable: 4996)

// 4661 allows setjmp to compile. I have gone at lengths to make sure it does not cause issues with C++ where it is used in libJPG and I see no evidence of vulnerability
# pragma warning(disable: 4611)
#else
#endif
