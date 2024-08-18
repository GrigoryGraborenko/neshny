////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#define NESHNY_TESTING

namespace Test {

	struct InfoException : public std::exception {
		std::string p_Info;
		InfoException(std::string info) : p_Info(info) {}
		~InfoException() throw () {}
		const char* what() const throw() { return p_Info.c_str(); }
	};
	inline void Expect(const char* info, bool condition) {
		if (!condition) {
			throw info;
		}
	}
	inline void Expect(std::string info, bool condition) {
		if (!condition) {
			throw InfoException(info);
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
		bool		p_Success;
		std::string	p_Label;
		std::string p_Error;
	};

								UnitTester	( void ) {}

	void						IRender		( void );
	void						IExecute	( void );

	TestResult					ExecuteTest	( const struct UnitTest& test );

	std::vector<TestResult>		m_Results;
	bool						m_Display = false;
};
