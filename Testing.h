////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#define NESHNY_TESTING

namespace Test {

	struct InfoException : public std::exception {
		QByteArray p_Info;
		InfoException(QString info) : p_Info(info.toLocal8Bit()) {}
		~InfoException() throw () {}
		const char* what() const throw() { return p_Info.data(); }
	};
	inline void Expect(const char* info, bool condition) {
		if (!condition) {
			throw info;
		}
	}
	inline void Expect(QString info, bool condition) {
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
		bool	p_Success;
		QString p_Label;
		QString p_Error;
	};

								UnitTester	( void ) {}

	void						IRender		( void );
	void						IExecute	( void );

	TestResult					ExecuteTest	( const struct UnitTest& test );

	std::vector<TestResult>		m_Results;
	bool						m_Display = false;
};
