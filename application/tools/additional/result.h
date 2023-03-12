#ifndef TOOLS_RESULT
#define TOOLS_RESULT

namespace result
{
    enum DataType
    {
        Ok = 0,
        Err = 1,
    };

    template <class T, class E>
    struct Result
    {
    private:
        DataType m_type;
        T m_data;
        E m_error;

    public:
        Result(T data) : m_type(DataType::Ok), m_data(data) {}
        Result(E err) : m_type(DataType::Err), m_error(err) {}

        bool isOk(void) { return (m_type == DataType::Ok); }
        bool isErr(void) { return (m_type == DataType::Err); }

        // Note: make sure to check if isOk() before trying to getData()
        T getData(void) { return m_data; }

        // Note: make sure to check if isErr() before trying to getErr()
        E getErr(void) { return m_error; }
    };
}

#endif // TOOLS_RESULT