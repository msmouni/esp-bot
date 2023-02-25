struct ServerLogin
{
    const char *login;
    const char *password;

    /// (login: "super_admin", password: "super_password")
    ServerLogin(const char *login = "super_admin", const char *password = "super_password") : login(login), password(password){};
};