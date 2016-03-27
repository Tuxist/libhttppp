namespace libhttppp{
  class Session {
  private:
  char  *_Sessionid; //shasum poolid+userid+time
  long   _Poolid;
  long   _UserID;
  };
};