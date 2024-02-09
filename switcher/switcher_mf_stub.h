#define FUNC0(_fn, _ret) static __stdcall DWORD _fn ## _stub() { return _ret; }
#define FUNC1(_fn, _ret) static __stdcall DWORD _fn ## _stub(DWORD a) { (void)a; return _ret; }
#define FUNC2(_fn, _ret) static __stdcall DWORD _fn ## _stub(DWORD a, DWORD b) { (void)a; (void)b; return _ret; }
#define FUNC3(_fn, _ret) static __stdcall DWORD _fn ## _stub(DWORD a, DWORD b, DWORD c) { (void)a; (void)b; (void)c; return _ret; }
#define FUNC4(_fn, _ret) static __stdcall DWORD _fn ## _stub(DWORD a, DWORD b, DWORD c, DWORD d) { (void)a; (void)b; (void)c; (void)d; return _ret; }
#define FUNC5(_fn, _ret) static __stdcall DWORD _fn ## _stub(DWORD a, DWORD b, DWORD c, DWORD d, DWORD e) { (void)a; (void)b; (void)c; (void)d; (void)e; return _ret; }
#define FUNC6(_fn, _ret) static __stdcall DWORD _fn ## _stub(DWORD a, DWORD b, DWORD c, DWORD d, DWORD e, DWORD f) { (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; return _ret; }
#define FUNC7(_fn, _ret) static __stdcall DWORD _fn ## _stub(DWORD a, DWORD b, DWORD c, DWORD d, DWORD e, DWORD f, DWORD g) { (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; return _ret; }
#define FUNC8(_fn, _ret) static __stdcall DWORD _fn ## _stub(DWORD a, DWORD b, DWORD c, DWORD d, DWORD e, DWORD f, DWORD g, DWORD h) { (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; (void)h; return _ret; } /* looks like I need larger screen */
#define FUNC9(_fn, _ret) static __stdcall DWORD _fn ## _stub(DWORD a, DWORD b, DWORD c, DWORD d, DWORD e, DWORD f, DWORD g, DWORD h, DWORD i) \
	{ (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; (void)h; (void)i; return _ret; }
#define FUNC10(_fn, _ret) static __stdcall DWORD _fn ## _stub(DWORD a, DWORD b, DWORD c, DWORD d, DWORD e, DWORD f, DWORD g, DWORD h, DWORD i, DWORD j) \
	{ (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; (void)h; (void)i; (void)j; return _ret; }
#define FUNC11(_fn, _ret) static __stdcall DWORD _fn ## _stub(DWORD a, DWORD b, DWORD c, DWORD d, DWORD e, DWORD f, DWORD g, DWORD h, DWORD i, DWORD j, DWORD k) \
	{ (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; (void)h; (void)i; (void)j; (void)g; (void)k; return _ret; }
#define FUNC12(_fn, _ret) static __stdcall DWORD _fn ## _stub(DWORD a, DWORD b, DWORD c, DWORD d, DWORD e, DWORD f, DWORD g, DWORD h, DWORD i, DWORD j, DWORD k, DWORD l) \
	{ (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; (void)h; (void)i; (void)j; (void)g; (void)k; (void)l; return _ret; }
#define FUNC13(_fn, _ret) static __stdcall DWORD _fn ## _stub(DWORD a, DWORD b, DWORD c, DWORD d, DWORD e, DWORD f, DWORD g, DWORD h, DWORD i, DWORD j, DWORD k, DWORD l, DWORD m) \
	{ (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; (void)h; (void)i; (void)j; (void)g; (void)k; (void)l; (void)m; return _ret; }
#define FUNC14(_fn, _ret) static __stdcall DWORD _fn ## _stub(DWORD a, DWORD b, DWORD c, DWORD d, DWORD e, DWORD f, DWORD g, DWORD h, DWORD i, DWORD j, DWORD k, DWORD l, DWORD m, DWORD n) \
	{ (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; (void)h; (void)i; (void)j; (void)g; (void)k; (void)l; (void)m; (void)n; return _ret; }
#define FUNC15(_fn, _ret) static __stdcall DWORD _fn ## _stub(DWORD a, DWORD b, DWORD c, DWORD d, DWORD e, DWORD f, DWORD g, DWORD h, DWORD i, DWORD j, DWORD k, DWORD l, DWORD m, DWORD n, DWORD o) \
	{ (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; (void)h; (void)i; (void)j; (void)g; (void)k; (void)l; (void)m; (void)n; (void)o; return _ret; }
