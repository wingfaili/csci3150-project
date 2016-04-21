#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#define EOC 0x0ffffff8
#pragma pack(push,1)
unsigned int *fat_disk;
unsigned int total_dir_entry;
unsigned int offset;//sub directory,number=no.of sub-directory

struct BootEntry {
	unsigned char BS_jmpBoot[3]; /* Assembly instruction to jump to boot code */
	unsigned char BS_OEMName[8]; /* OEM Name in ASCII */
	unsigned short BPB_BytsPerSec; /* Bytes per sector. Allowed values include 512, 1024, 2048, and 4096 */
	unsigned char BPB_SecPerClus; /* Sectors per cluster (data unit). Allowed values are powers of 2, but the cluster size must be 32KB or smaller */
	unsigned short BPB_RsvdSecCnt; /* Size in sectors of the reserved area */
	unsigned char BPB_NumFATs; /* Number of FATs */
	unsigned short BPB_RootEntCnt; /* Maximum number of files in the root directory for FAT12 and FAT16. This is 0 for FAT32 */
	unsigned short BPB_TotSec16; /* 16-bit value of number of sectors in file system */
	unsigned char BPB_Media; /* Media type */
	unsigned short BPB_FATSz16; /* 16-bit size in sectors of each FAT for FAT12 and FAT16. For FAT32, this field is 0 */
	unsigned short BPB_SecPerTrk; /* Sectors per track of storage device */
	unsigned short BPB_NumHeads; /* Number of heads in storage device */
	unsigned int BPB_HiddSec; /* Number of sectors before the start of partition */
	unsigned int BPB_TotSec32; /* 32-bit value of number of sectors in file system. Either this value or the 16-bit value above must be 0 */
	unsigned int BPB_FATSz32; /* 32-bit size in sectors of one FAT */
	unsigned short BPB_ExtFlags; /* A flag for FAT */
	unsigned short BPB_FSVer; /* The major and minor version number */
	unsigned int BPB_RootClus; /* Cluster where the root directory can be found */
	unsigned short BPB_FSInfo; /* Sector where FSINFO structure can be found */
	unsigned short BPB_BkBootSec; /* Sector where backup copy of boot sector is located */
	unsigned char BPB_Reserved[12]; /* Reserved */
	unsigned char BS_DrvNum; /* BIOS INT13h drive number */
	unsigned char BS_Reserved1; /* Not used */
	unsigned char BS_BootSig; /* Extended boot signature to identify if the next three values are valid */
	unsigned int BS_VolID; /* Volume serial number */
	unsigned char BS_VolLab[11]; /* Volume label in ASCII. User defines when creating the file system */
	unsigned char BS_FilSysType[8]; /* File system type label in ASCII */
};
struct DirEntry{
	unsigned char DIR_Name[11]; /* File name */
	unsigned char DIR_Attr; /* File attributes */
	unsigned char DIR_NTRes; /* Reserved */
	unsigned char DIR_CrtTimeTenth; /* Created time (tenths of second) */
	unsigned short DIR_CrtTime; /* Created time (hours, minutes, seconds) */
	unsigned short DIR_CrtDate; /* Created day */
	unsigned short DIR_LstAccDate; /* Accessed day */
	unsigned short DIR_FstClusHI; /* High 2 bytes of the first cluster address */
	unsigned short DIR_WrtTime; /* Written time (hours, minutes, seconds */
	unsigned short DIR_WrtDate; /* Written day */
	unsigned short DIR_FstClusLO; /* Low 2 bytes of the first cluster address */
	unsigned int DIR_FileSize; /* File size in bytes. (0 for directories) */
};

struct DirEntry *dir;
struct BootEntry boot;
#pragma pack(pop)

void file_name(unsigned i,char *tmp){
	int j;
	for (j=0; j<8; j++) {
		if (dir[i].DIR_Name[j] == ' ') {
			break;
		}
		*tmp++ = dir[i].DIR_Name[j];
	}
	if (dir[i].DIR_Name[8] != ' ') {
		*tmp++ = '.';
		for (j=8; j<11; j++) {
			if (dir[i].DIR_Name[j] == ' '){
				break;
			}
			*tmp++ = dir[i].DIR_Name[j];
		}
	}
	if(dir[i].DIR_Attr & 0x10){
			*tmp++ ='/';
			//printf("%c\n", dir[i].DIR_Attr);
	}
	*tmp='\0';
}

void print_directory(){
	unsigned int i,fsize,start;
	char fname[257],fname2[257];//fname2=deleted file
	int no=1;
	for(i=0;i<total_dir_entry;i++){
		if ( dir[i].DIR_Attr == 0x0f||dir[i].DIR_Name[0] ==0x00) continue;
		file_name(i,fname);
		fsize = dir[i].DIR_FileSize;
		start = (dir[i].DIR_FstClusHI<<16)+dir[i].DIR_FstClusLO;
		if(dir[i].DIR_Name[0]==0xe5){//deleted file
			file_name(i,fname2);
			fname2[0]='?';
			if(fname2[6]=='~'){
				printf("%d, LFN entry\n", no++);
			}
			printf("%d, %s, %u, %u\n",no++,fname2,fsize,start);
		}
		else{
			if(fname[6]=='~'){
				printf("%d, LFN entry\n", no++);
			}
			printf("%d, %s, %u, %u\n",no++,fname,fsize,start);
		}
	}
}

void readbootentry(FILE *fptr){
		unsigned int root_cluster, i;
		fread(&boot, sizeof(struct BootEntry), 1, fptr);
		fat_disk = malloc(boot.BPB_FATSz32*boot.BPB_BytsPerSec);
		pread(fileno(fptr), fat_disk, boot.BPB_FATSz32*boot.BPB_BytsPerSec, boot.BPB_RsvdSecCnt*boot.BPB_BytsPerSec);
		root_cluster=0;
		for (i=boot.BPB_RootClus; i<EOC; i=fat_disk[i]){
			root_cluster++;
		}

		struct DirEntry *ter;
		ter = dir = malloc(root_cluster*boot.BPB_BytsPerSec*boot.BPB_SecPerClus);
		total_dir_entry = (root_cluster*boot.BPB_BytsPerSec*boot.BPB_SecPerClus)/sizeof(struct DirEntry);
		offset=(boot.BPB_RsvdSecCnt+boot.BPB_NumFATs*boot.BPB_FATSz32)*boot.BPB_BytsPerSec;
		for(i=boot.BPB_RootClus;i<EOC;i=fat_disk[i]){
			pread(fileno(fptr), ter, boot.BPB_BytsPerSec*boot.BPB_SecPerClus, offset+(i-2)*boot.BPB_BytsPerSec*boot.BPB_SecPerClus);
			ter += (boot.BPB_BytsPerSec*boot.BPB_SecPerClus)/sizeof(struct DirEntry);
		}
		//fclose(fptr);
}

void recovery(FILE *fptr, char* target, char* dest){
	unsigned int i, match = -1;
	unsigned int fsize,start;
	char fname[257];
	for (i=0; i<total_dir_entry; i++) {
		if (dir[i].DIR_Name[0] != 0xe5 || dir[i].DIR_Attr == 0x0f || dir[i].DIR_Attr == 0x10) continue;
			file_name(i, fname);
			if (strcmp(fname+1, target+1) == 0) {
				if (match == -1) match = i;
			}
	}
	if (match == -1)
		printf("%s: error - file not found\n", target);
	else {
		start = (dir[match].DIR_FstClusHI<<16)+dir[match].DIR_FstClusLO;
		fsize = dir[match].DIR_FileSize;
		if (fsize != 0 && fat_disk[start] != 0)
			printf("%s: error - fail to recover\n", target);
		else {
			FILE *op = fopen(dest,"w");
			if (op == NULL) {
				printf("%s: failed to open\n", dest);
			}
			else {
				void *buf = malloc(fsize);
				pread(fileno(fptr), buf, fsize, offset+(start-2)*boot.BPB_BytsPerSec*boot.BPB_SecPerClus);
				fwrite(buf, fsize, 1, op);
				printf("%s: recovered\n", target);
				fclose(op);
			}
		}
	}
}

void cleanse(FILE *fptr, char* target){
	unsigned int i, match = -1;
	unsigned int fsize,start;
	char fname[257];
	for (i=0; i<total_dir_entry; i++) {
		if (dir[i].DIR_Name[0] != 0xe5 || dir[i].DIR_Attr == 0x0f || dir[i].DIR_Attr == 0x10) continue;
			file_name(i, fname);
			if (strcmp(fname+1, target+1) == 0) {
				if (match == -1) match = i;
			}
	}
	if (match == -1)
		printf("%s: error - file not found\n", target);
	else {
		start = (dir[match].DIR_FstClusHI<<16)+dir[match].DIR_FstClusLO;
		fsize = dir[match].DIR_FileSize;
		if (fsize == 0 && fat_disk[start] != 0)
			printf("%s: error - fail to cleanse\n", target);
		else {
			//char *buf = (char*) malloc(fsize);
			const void *buf = calloc(fsize, sizeof(unsigned int));
			//unsigned int j=0;
			// char zero[512] = {0};
			//printf("%u\n", fsize);
			// for(j=0; j<fsize; j++){
			// 	*buf++ = '0';
			// }

			pwrite(fileno(fptr), buf, fsize, offset+(start-2)*boot.BPB_BytsPerSec*boot.BPB_SecPerClus);
			printf("%s: cleansed\n", target);
		}
	}
}

void print_opt(char* argv){
	printf("Usage: %s -d [device filename] [other arguments]\n", argv);
	printf("-i\t\t\tPrint file system information\n");
	printf("-l\t\t\tList the root directory\n");
	printf("-r target -o dest\tRecover the target deleted file\n");
	printf("-x target\t\tCleanse the target deleted file\n");
}

void print_info(){
	printf("Number of FATs = %d\n", boot.BPB_NumFATs);
	printf("Number of bytes per sector = %d\n", boot.BPB_BytsPerSec);
	printf("Number of sectors per cluster = %d\n", boot.BPB_SecPerClus);
	printf("Number of reserved sectors = %d\n", boot.BPB_RsvdSecCnt);
	printf("First FAT starts at byte = %d\n", boot.BPB_BytsPerSec*boot.BPB_RsvdSecCnt);
	printf("Data area starts at byte = %d\n", (boot.BPB_BytsPerSec*boot.BPB_RsvdSecCnt)+boot.BPB_FATSz32*boot.BPB_NumFATs*boot.BPB_BytsPerSec);
}

int main(int argc, char** argv){
  char c;
	int vaild = 0;
	FILE *fptr;
	if(argc>=2 && strcmp(argv[1], "-d")==0){
	  while((c=getopt(argc, argv, "d:ilr:o:x:"))!= -1){
			switch (c) {
	      case 'd':
				 if(argv[2]!=NULL){
					 if(argv[3]!=NULL && (strcmp(argv[3], "-i")==0 || strcmp(argv[3], "-l")==0 || strcmp(argv[3], "-r")==0 || strcmp(argv[3], "-x")==0)){
						 vaild=1;
						 fptr = fopen(argv[2], "r+");
						 if(fptr==NULL){
							 perror("Error");
							 exit(1);
						 }
						 readbootentry(fptr); //with fread() but fclose() inside readbootentry().
					 }
					 else{
						 print_opt(argv[0]);
						 exit(1);
					 }
				 }
				 break;
				case 'i':
					if(vaild){
						print_info();
						fclose(fptr);
					}
					else{
						print_opt(argv[0]);
						exit(1);
					}
					break;
				case 'l':
					if(vaild){
						print_directory();
						fclose(fptr);
						break;
					}
					else{
						print_opt(argv[0]);
						exit(1);
					}
					break;
				case 'r':
					if(vaild){
						if(argv[5]==NULL || (argv[5]!=NULL && strcmp(argv[5], "-o")!=0)){
							print_opt(argv[0]);
							fclose(fptr);
							exit(1);
						}
					}
					else{
						print_opt(argv[0]);
						exit(1);
					}
					break;
				case 'o':
					if(vaild && argv[6]!=NULL){
						recovery(fptr, argv[4], argv[6]);
						fclose(fptr);
					}
					else{
						print_opt(argv[0]);
						exit(1);
					}
					break;
				case 'x':
					if(vaild){
						cleanse(fptr, argv[4]);
						fclose(fptr);
					}
					else{
						print_opt(argv[0]);
						exit(1);
					}
					break;
	    }
	  }
	}
  else{
		print_opt(argv[0]);
		exit(1);
	}
  return 0;
}
