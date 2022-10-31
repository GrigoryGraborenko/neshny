////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Test {

//#define EXPECT(x, y) (_Expect(__func__, (x), (y)))

	//inline void _Expect(const char* func_name, const char* info, bool condition) {
	inline void Expect(const char* info, bool condition) {
		if (!condition) {
			throw info;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class UnitTester {

public:

	static inline UnitTester&	Singleton		( void ) { static UnitTester instance; return instance; }

	static inline void			RenderImGui		( void ) { Singleton().IRender(); }
	static inline void			Execute			( void ) { Singleton().IExecute(); }

protected:

	struct TestResult {
		bool	p_Success;
		QString p_Label;
		QString p_Error;
	};

								UnitTester	( void ) {}

	void						IRender		( void );
	void						IExecute	( void );

	std::vector<TestResult>		m_Results;
	bool						m_Display = false;
};
