#include <kernel/OS.h>
#define USE_POSIX_TIME
#define HAS_TEST_AND_SET

typedef unsigned char slock_t;

#define AF_UNIX     1 /* no domain sockets on BeOS */

#ifdef __cplusplus
extern "C" {
#endif
	#include "kernel/image.h"
	
	#undef HAVE_UNION_SEMUN
	#define HAVE_UNION_SEMUN 1
	#undef HAVE_SYS_SEM_H
	#undef HAVE_SYS_SHM_H

	union semun
	{
		int			val;
		struct semid_ds *buf;
		unsigned short *array;
	};
	/* SYS V emulation */

	#define IPC_RMID 256
	#define IPC_CREAT 512
	#define IPC_EXCL 1024
	#define IPC_PRIVATE 234564

	#define EACCESS 2048
	#define EIDRM 4096

	#define SETALL 8192
	#define GETNCNT 16384
	#define GETVAL 65536
	#define SETVAL 131072

	struct sembuf
	{
		int sem_flg;
		int sem_op;
		int sem_num;
	};

	int semctl(int semId,int semNum,int flag,union semun);
	int semget(int semKey, int semNum, int flags);
	int semop(int semId, struct sembuf *sops, int flag);

	struct shmid_ds
	{
		int			dummy;
	};
	
	int shmdt(char* shmaddr);
	int* shmat(int memId,int m1,int m2);
	int shmctl(int shmid,int flag, struct shmid_ds* dummy);
	int shmget(int memKey,int size,int flag);


	/* Support functions */
	/* Specific beos action made on postgres/postmaster startup */
	void beos_startup(int argc,char** argv);
	/* Load a shared library */
	image_id beos_dl_open(char * filename);
	/* UnLoad a shared library */
	status_t beos_dl_close(image_id im);
	/* Specific beos action made on backend startup */
	void beos_backend_startup(char* binary);
#ifdef __cplusplus
}
#endif