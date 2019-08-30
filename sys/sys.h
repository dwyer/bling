#pragma once
#include "bootstrap/bootstrap.h"

package(sys);

typedef u64 sys$Size;

extern int sys$errno();
extern void sys$errnoReset();
extern char *sys$errnoString();

extern char *sys$getenv(const char *key);

// stdio
extern void sys$printf(const char *s, ...);
extern char *sys$sprintf(const char *s, ...);

// stdlib
extern void *sys$malloc(sys$Size n);
extern void sys$free(void *ptr);
extern void *sys$realloc(void *ptr, sys$Size n);

// strings
extern int sys$memcmp(const void *dst, const void *src, sys$Size n);
extern void *sys$memcpy(void *dst, const void *src, sys$Size n);
extern void *sys$memset(void *dst, int c, sys$Size n);
extern char *sys$strdup(const char *s);
extern char *sys$strndup(const char *s, sys$Size n);
extern sys$Size sys$strlen(const char *s);

extern bool sys$streq(const char *a, const char *b);

extern int sys$open(const char *filename, int oflag, int perm);
extern int sys$close(int fd);
extern int sys$read(int fd, void *b, sys$Size n);
extern int sys$write(int fd, const void *b, sys$Size n);

typedef enum {

    sys$O_RDONLY    = 0x0000,
    sys$O_WRONLY    = 0x0001,
    sys$O_RDWR      = 0x0002,

    sys$O_APPEND   =  0x0008,
    sys$O_CREAT     = 0x0200,
    sys$O_TRUNC     = 0x0400,
    sys$O_EXCL      = 0x0800,

} sys$FileOpenMode;

typedef enum {
    sys$STDIN_FILENO = 0,
    sys$STDOUT_FILENO = 1,
    sys$STDERR_FILENO = 2,
} sys$FileNo;

typedef enum {
    sys$E2BIG           = 0x7,
    sys$EACCES          = 0xd,
    sys$EADDRINUSE      = 0x62,
    sys$EADDRNOTAVAIL   = 0x63,
    sys$EADV            = 0x44,
    sys$EAFNOSUPPORT    = 0x61,
    sys$EAGAIN          = 0xb,
    sys$EALREADY        = 0x72,
    sys$EBADE           = 0x34,
    sys$EBADF           = 0x9,
    sys$EBADFD          = 0x4d,
    sys$EBADMSG         = 0x4a,
    sys$EBADR           = 0x35,
    sys$EBADRQC         = 0x38,
    sys$EBADSLT         = 0x39,
    sys$EBFONT          = 0x3b,
    sys$EBUSY           = 0x10,
    sys$ECANCELED       = 0x7d,
    sys$ECHILD          = 0xa,
    sys$ECHRNG          = 0x2c,
    sys$ECOMM           = 0x46,
    sys$ECONNABORTED    = 0x67,
    sys$ECONNREFUSED    = 0x6f,
    sys$ECONNRESET      = 0x68,
    sys$EDEADLK         = 0x23,
    sys$EDEADLOCK       = 0x23,
    sys$EDESTADDRREQ    = 0x59,
    sys$EDOM            = 0x21,
    sys$EDOTDOT         = 0x49,
    sys$EDQUOT          = 0x7a,
    sys$EEXIST          = 0x11,
    sys$EFAULT          = 0xe,
    sys$EFBIG           = 0x1b,
    sys$EHOSTDOWN       = 0x70,
    sys$EHOSTUNREACH    = 0x71,
    sys$EIDRM           = 0x2b,
    sys$EILSEQ          = 0x54,
    sys$EINPROGRESS     = 0x73,
    sys$EINTR           = 0x4,
    sys$EINVAL          = 0x16,
    sys$EIO             = 0x5,
    sys$EISCONN         = 0x6a,
    sys$EISDIR          = 0x15,
    sys$EISNAM          = 0x78,
    sys$EKEYEXPIRED     = 0x7f,
    sys$EKEYREJECTED    = 0x81,
    sys$EKEYREVOKED     = 0x80,
    sys$EL2HLT          = 0x33,
    sys$EL2NSYNC        = 0x2d,
    sys$EL3HLT          = 0x2e,
    sys$EL3RST          = 0x2f,
    sys$ELIBACC         = 0x4f,
    sys$ELIBBAD         = 0x50,
    sys$ELIBEXEC        = 0x53,
    sys$ELIBMAX         = 0x52,
    sys$ELIBSCN         = 0x51,
    sys$ELNRNG          = 0x30,
    sys$ELOOP           = 0x28,
    sys$EMEDIUMTYPE     = 0x7c,
    sys$EMFILE          = 0x18,
    sys$EMLINK          = 0x1f,
    sys$EMSGSIZE        = 0x5a,
    sys$EMULTIHOP       = 0x48,
    sys$ENAMETOOLONG    = 0x24,
    sys$ENAVAIL         = 0x77,
    sys$ENETDOWN        = 0x64,
    sys$ENETRESET       = 0x66,
    sys$ENETUNREACH     = 0x65,
    sys$ENFILE          = 0x17,
    sys$ENOANO          = 0x37,
    sys$ENOBUFS         = 0x69,
    sys$ENOCSI          = 0x32,
    sys$ENODATA         = 0x3d,
    sys$ENODEV          = 0x13,
    sys$ENOENT          = 0x2,
    sys$ENOEXEC         = 0x8,
    sys$ENOKEY          = 0x7e,
    sys$ENOLCK          = 0x25,
    sys$ENOLINK         = 0x43,
    sys$ENOMEDIUM       = 0x7b,
    sys$ENOMEM          = 0xc,
    sys$ENOMSG          = 0x2a,
    sys$ENONET          = 0x40,
    sys$ENOPKG          = 0x41,
    sys$ENOPROTOOPT     = 0x5c,
    sys$ENOSPC          = 0x1c,
    sys$ENOSR           = 0x3f,
    sys$ENOSTR          = 0x3c,
    sys$ENOSYS          = 0x26,
    sys$ENOTBLK         = 0xf,
    sys$ENOTCONN        = 0x6b,
    sys$ENOTDIR         = 0x14,
    sys$ENOTEMPTY       = 0x27,
    sys$ENOTNAM         = 0x76,
    sys$ENOTRECOVERABLE = 0x83,
    sys$ENOTSOCK        = 0x58,
    sys$ENOTSUP         = 0x5f,
    sys$ENOTTY          = 0x19,
    sys$ENOTUNIQ        = 0x4c,
    sys$ENXIO           = 0x6,
    sys$EOPNOTSUPP      = 0x5f,
    sys$EOVERFLOW       = 0x4b,
    sys$EOWNERDEAD      = 0x82,
    sys$EPERM           = 0x1,
    sys$EPFNOSUPPORT    = 0x60,
    sys$EPIPE           = 0x20,
    sys$EPROTO          = 0x47,
    sys$EPROTONOSUPPORT = 0x5d,
    sys$EPROTOTYPE      = 0x5b,
    sys$ERANGE          = 0x22,
    sys$EREMCHG         = 0x4e,
    sys$EREMOTE         = 0x42,
    sys$EREMOTEIO       = 0x79,
    sys$ERESTART        = 0x55,
    sys$ERFKILL         = 0x84,
    sys$EROFS           = 0x1e,
    sys$ESHUTDOWN       = 0x6c,
    sys$ESOCKTNOSUPPORT = 0x5e,
    sys$ESPIPE          = 0x1d,
    sys$ESRCH           = 0x3,
    sys$ESRMNT          = 0x45,
    sys$ESTALE          = 0x74,
    sys$ESTRPIPE        = 0x56,
    sys$ETIME           = 0x3e,
    sys$ETIMEDOUT       = 0x6e,
    sys$ETOOMANYREFS    = 0x6d,
    sys$ETXTBSY         = 0x1a,
    sys$EUCLEAN         = 0x75,
    sys$EUNATCH         = 0x31,
    sys$EUSERS          = 0x57,
    sys$EWOULDBLOCK     = 0xb,
    sys$EXDEV           = 0x12,
    sys$EXFULL          = 0x36,
} sys$Errno;

typedef void *sys$Dir;
extern sys$Dir sys$opendir(const char *filename);
extern int sys$closedir(sys$Dir dir);

typedef void *sys$Dirent;
extern sys$Dirent sys$readdir(sys$Dir dir);
extern char *sys$Dirent_name(sys$Dirent dirent);

typedef struct {
    u32 mode;
    i32 size;
    u64 atime;
    u64 mtime;
    u64 ctime;
} sys$Stat;

extern int sys$stat(const char *path, sys$Stat *s);

typedef enum {
    sys$S_IXOTH     = 0001,
    sys$S_IWOTH     = 0002,
    sys$S_IROTH     = 0004,

    sys$S_IXGRP     = 0010,
    sys$S_IWGRP     = 0020,
    sys$S_IRGRP     = 0040,

    sys$S_IXUSR     = 0100,
    sys$S_IWUSR     = 0200,
    sys$S_IRUSR     = 0400,

    sys$S_IRWXO     = sys$S_IXOTH | sys$S_IWOTH | sys$S_IROTH,
    sys$S_IRWXG     = sys$S_IXGRP | sys$S_IWGRP | sys$S_IRGRP,
    sys$S_IRWXU     = sys$S_IXUSR | sys$S_IWUSR | sys$S_IRUSR,

    sys$S_BLKSIZE   = 0x200,

    sys$S_IEXEC     = 0x40,
    sys$S_IWRITE    = 0x80,
    sys$S_IREAD     = 0x100,
    sys$S_ISVTX     = 0x200,
    sys$S_ISGID     = 0x400,
    sys$S_ISUID     = 0x800,
    sys$S_IFIFO     = 0x1000,
    sys$S_IFCHR     = 0x2000,
    sys$S_IFDIR     = 0x4000,
    sys$S_IFBLK     = 0x6000,
    sys$S_IFREG     = 0x8000,
    sys$S_IFLNK     = 0xa000,
    sys$S_IFSOCK    = 0xc000,
    sys$S_IFMT      = 0xf000,

} sys$S;

extern int sys$mkdir(const char *path, u32 mode);
extern char **sys$environ();

typedef int sys$Pid;
extern int sys$execve(const char *path, char *const argv[], char *const envp[]);
extern sys$Pid sys$fork();
extern sys$Pid sys$waitpid(sys$Pid pid, int *status, int opts);
