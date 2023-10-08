/*--------------------------------*/
/* Get size of directory in bytes */
/*--------------------------------*/

_PUBLIC off_t pus_dsize(char *directory_name)
{
    off_t directory_size = 0;
    DIR   *pDir          = (DIR *)NULL;

    if ((pDir = opendir(directory_name)) != (DIR *)NULL)
    {
        struct dirent *pDirent = (struct dirent *)NULL;

        /*--------------------------------*/
	/* Get size of files in directory */
	/*--------------------------------*/

        while ((pDirent = readdir(pDir)) != (struct dirent *)NULL)
        {
            char   buffer[PATH_MAX + 1] = "";
            struct stat file_stat;

            (void)strcat(strcat(strcpy(buffer, directory_name), "/"), pDirent->d_name);

            if (stat(buffer, &file_stat) == 0)
                directory_size += file_stat.st_blocks * S_BLKSIZE;


	    /*-----------------*/
	    /* Sub-directory ? */
	    /*-----------------*/

            if (pDirent->d_type == DT_DIR)
            {  

	       /*-----*/
	       /* Yes */
	       /*-----*/

	       if (strcmp(pDirent->d_name, ".") != 0 && strcmp(pDirent->d_name, "..") != 0)
                    directory_size += pups_dsize(buffer);
            }
        }

        (void) closedir(pDir);
    }


    /*------------------------*/
    /* Size of directory tree */
    /* in bytes               */
    /*------------------------*/

    return(directory_size);
}
