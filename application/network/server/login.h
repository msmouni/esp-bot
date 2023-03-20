#ifndef SERVER_LOGIN
#define SERVER_LOGIN

struct Login
{
    char m_login_password[127];

    /// (login: "login", password: "password")
    Login(const char *login = "login", const char *password = "password")
    {
        sprintf(m_login_password, "{%s:%s}", login, password);
    };

    bool check(char *login_password)
    {
        for (int index = 0; index < sizeof(m_login_password); index++)
        {
            // printf("index:%d lp %c, m_lp %c\n", index, *login_password, m_login_password[index]);
            if (*login_password != m_login_password[index])
            {
                return false;
            }
            else if (m_login_password[index] == '\0' && *login_password == '\0')
            {
                return true;
            }
            login_password++;
        }
        return false;
    }
};

enum class LoginResult
{
    Client,
    SuperClient,
    WrongLogin,
};

struct ServerLogin
{
    Login m_connection_login;
    Login m_sudo_login;

    ServerLogin(Login connection_login = {}, Login sudo_login = {}) : m_connection_login(connection_login), m_sudo_login(sudo_login){};

    LoginResult check(char *login_password)
    {
        if (m_sudo_login.check(login_password))
        {
            return LoginResult::SuperClient;
        }
        else if (m_connection_login.check(login_password))
        {
            return LoginResult::Client;
        }
        else
        {
            return LoginResult::WrongLogin;
        }
    }
};

#endif // SERVER_LOGIN