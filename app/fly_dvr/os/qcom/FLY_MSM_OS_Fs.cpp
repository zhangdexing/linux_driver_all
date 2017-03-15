#include "Flydvr_Common.h"
#include <dirent.h>

int FLY_MSM_OS_CpFile(char *srcFile, char *targetFile, int findString, char *string)
    {
        int ret = 0;
        FILE *srcfp = NULL;
        FILE *targetfp = NULL;
        int length = 0;
        ssize_t size;
        size_t len = 0;
        char *p = NULL;
        char *line = NULL;

        if(access(srcFile, F_OK) != 0)
        {
            printf("ERROR: src file is not exist!");
            return -1;
        }
        if(ret == 0)
        {
            srcfp = fopen(srcFile, "r");
            targetfp = fopen(targetFile, "w+");
            if((targetfp != NULL) && (srcfp != NULL))
            {
                while((size = getline(&line, &len, srcfp)) != -1)
                {
                    if(size > 0)
                    {
                        if(findString == 1)
                        {
                            p = strstr(line, string);
                            if(p != NULL)
                            {
                                length = fwrite(line, size, 1, targetfp);
                            }
                        }
                        else
                        {
                            length = fwrite(line, size, 1, targetfp);
                        }
                    }
                }
                fclose(srcfp);
                fclose(targetfp);
                if(line)
                {
                    free(line);
                }
                ret = 0;
            }
            else
            {
                if(srcfp)
                {
                    printf("ERROR: open flie %s", targetFile);
                    fclose(srcfp);
                }
                if(targetfp)
                {
                    printf("ERROR: open flie %s", srcFile);
                    fclose(targetfp);
                }
                ret = -1;
            }
        }
        return ret;
}


bool FLY_MSM_OS_IsFileExist(const char *file_path)
{
	if(file_path == NULL)
		return false;
	if(access(file_path, F_OK) == 0)
		return true;
	return false;
}

bool FLY_MSM_OS_IsDirExist(const char *dir_path)
{
	DIR *pDir;  
	if(dir_path == NULL)
		return false;
	pDir = opendir(dir_path);
	if(pDir == NULL)
		return false;
	else closedir(pDir);
	return true;
}

bool FLY_MSM_OS_MkDir(const char *dir_path)
{ 
	if(dir_path == NULL)
		return false;
	if(mkdir(dir_path,S_IRWXU|S_IRWXG|S_IRWXO) >= 0)
	{
		chmod(dir_path,0777);
		return true;
	}
	return false;
}

bool FLY_MSM_OS_ClearDir(const char *dir_path)
{ 
	char tmpCMD[255] = {0};
	if(dir_path == NULL)
		return false;
	sprintf(tmpCMD , "rm -rf %s/*&",dir_path);
	system(tmpCMD);
	return true;
}