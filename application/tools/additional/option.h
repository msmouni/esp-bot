#ifndef TOOLS_OPTION
#define TOOLS_OPTION

namespace option
{
    enum DataType
    {
        None = 0,
        Some = 1,
    };

    template <class T>
    struct Option
    {
    private:
        DataType m_type;
        T m_data;

    public:
        // T(): default constructed value for a given type.
        Option() : m_type(DataType::None), m_data(T()) {}

        Option(T data) : m_type(DataType::Some), m_data(data) {}

        bool isSome(void) { return (m_type == DataType::Some); }
        bool isNone(void) { return (m_type == DataType::None); }

        // Note: make sure to check if isSome() before trying to getData()
        T getData(void) { return m_data; }
    };
}

#endif // TOOLS_OPTION