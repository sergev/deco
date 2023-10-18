char **EnvVector;
void EnvInit(char **env);
char *EnvGet(char *name);
void EnvPut(char *name, char *value);
void EnvDelete(char *name);
