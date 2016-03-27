class User {
  
};

class Group {
  char *Name;
  int   gid;
};

class Auth {
public:
  Auth();
  ~Auth();
      
  bool canWrite();
  bool canExecute();
  bool canRead()
};
