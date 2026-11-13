// SPDX-License-Identifier: GPL-3.0-or-later
// Same source on native ARM64 Linux and Android Bionic/Trames.
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/xattr.h>
#include <unistd.h>
static void check(int ok,const char *step){if(!ok){perror(step);exit(1);}}
struct entry{uint16_t tag,perm;uint32_t id;};
struct acl{uint32_t version;struct entry entries[6];};
static int child_access(const char *file,int write_access){
 pid_t pid=fork();check(pid>=0,"fork");
 if(!pid){check(!setresgid(1000,1000,1000)&&!setresuid(1000,1000,1000),"drop ids");int fd=open(file,write_access?O_WRONLY:O_RDONLY);if(fd>=0)close(fd);_exit(fd>=0?0:errno==EACCES?2:3);}
 int status;check(waitpid(pid,&status,0)==pid&&WIFEXITED(status),"wait");return WEXITSTATUS(status);
}
int main(int argc,char **argv){
 char parent[512],dir[600],file[640];snprintf(parent,sizeof(parent),"%s/trames-acl-XXXXXX",argc>1?argv[1]:"/tmp");check(mkdtemp(parent)!=NULL,"mkdtemp");check(!chmod(parent,0777),"parent mode");
 struct acl acl={2,{{1,7,UINT32_MAX},{2,7,1000},{4,0,UINT32_MAX},{8,4,2000},{16,7,UINT32_MAX},{32,0,UINT32_MAX}}};
 check(!setxattr(parent,"system.posix_acl_default",&acl,sizeof(acl),0),"vold default ACL");
 umask(077);snprintf(dir,sizeof(dir),"%s/child",parent);check(!mkdir(dir,0777),"inherit directory");snprintf(file,sizeof(file),"%s/file",dir);int fd=open(file,O_CREAT|O_RDWR,0640);check(fd>=0,"inherit file");
 struct stat st;check(!fstat(fd,&st)&&(st.st_mode&0777)==0640,"ACL ignores umask and respects creation mode");
 struct acl copy;check(getxattr(dir,"system.posix_acl_default",&copy,sizeof(copy))==sizeof(copy)&&!memcmp(&copy,&acl,sizeof(copy)),"default inheritance");
 check(child_access(file,0)==0&&child_access(file,1)==2,"named user and mask");
 check(!fchmod(fd,0600)&&child_access(file,0)==2,"chmod updates ACL mask");
 acl.entries[0].perm=8;check(fsetxattr(fd,"system.posix_acl_access",&acl,sizeof(acl),0)<0&&errno==EINVAL,"invalid ACL");
 close(fd);check(!unlink(file)&&!rmdir(dir)&&!rmdir(parent),"cleanup");
 puts("android-acl: PASS vold default ACL, inheritance, named user permissions, chmod and validation");return 0;
}
