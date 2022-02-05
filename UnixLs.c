//References
//https://www.howtogeek.com/448446/how-to-use-the-ls-command-on-linux/
//https://stackoverflow.com/questions/2985224/how-to-implement-unix-ls-s-command-in-c
//https://unix.stackexchange.com/questions/3576/example-for-kernel-timer-implementation-in-linux
//https://github.com/junha3284/UnixLs
//https://linuxize.com/post/how-to-list-files-in-linux-using-the-ls-command/
//https://stackoverflow.com/questions/39432306/opening-a-file-using-relative-path

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>

#define SIZE 5000

//prints the letter corresponding the file's status
void getPermission(struct stat file_status)
{
    printf((S_ISDIR(file_status.st_mode)) ? "d" : "-");
    printf((file_status.st_mode & S_IRUSR) ? "r" : "-");
    printf((file_status.st_mode & S_IWUSR) ? "w" : "-");
    printf((file_status.st_mode & S_IXUSR) ? "x" : "-");
    printf((file_status.st_mode & S_IRGRP) ? "r" : "-");
    printf((file_status.st_mode & S_IWGRP) ? "w" : "-");
    printf((file_status.st_mode & S_IXGRP) ? "x" : "-");
    printf((file_status.st_mode & S_IROTH) ? "r" : "-");
    printf((file_status.st_mode & S_IWOTH) ? "w" : "-");
    printf((file_status.st_mode & S_IXOTH) ? "x" : "-");
    printf(" ");
}

void print_l(const char *dir_name, DIR *dirstream, struct dirent *directory, int option_i)
{
    char dir_path[SIZE];
    struct tm *time = NULL;
    char *months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    for (; directory; directory = readdir(dirstream))
    {
        char original[SIZE];
        struct stat file_status;

        snprintf(dir_path, sizeof(dir_path), "%s/%s", dir_name, directory->d_name);

        lstat(dir_path, &file_status);
        time = localtime(&file_status.st_mtime);

        if (directory->d_name[0] == '.')
            continue;
        //print inode number is requested
        if (option_i == 1)
        {
            printf("%lu ", directory->d_ino);
        }
        //permissions
        getPermission(file_status);
        //links
        printf("%ld ", file_status.st_nlink);
        //owner
        struct passwd *owner;
        owner = getpwuid(file_status.st_uid);
        printf("%s ", owner->pw_name);
        //group
        struct group *group;
        group = getgrgid(owner->pw_gid);
        printf("%s ", group->gr_name);
        //size
        printf("%5lu ", file_status.st_size);
        //last modified date
        printf("%s %2d %04d %02d:%02d ", months[time->tm_mon], time->tm_mday, time->tm_year + 1900, time->tm_hour, time->tm_min);
        //file name
        printf("%s", directory->d_name);
        //if the file/directory is a symbolic link, prints what it's pointing to
        if (S_ISLNK(file_status.st_mode))
        {
            char *sym_link = realpath(directory->d_name, original);
            printf(" -> %s", original);
        }
        printf("\n");
    }
    if (option_i == 1)
    {
        option_i = 0;
    }
}

void print_i(DIR *dirstream, struct dirent *directory)
{
    for (; directory; directory = readdir(dirstream))
    {
        if (directory->d_name[0] == '.')
            continue;
        printf("%lu ", directory->d_ino);
        printf("%s  ", directory->d_name);
    }
    printf("\n");
}

//recursively goes through all the paths of given directory and prints the files and directories
void print_R(const char *paths, struct dirent *directory, int option_l, int option_i)
{
    DIR *dirstream;
    if (!(dirstream = opendir(paths)))
        return;
    while ((directory = readdir(dirstream)) != NULL)
    {
        //prints out the file/directiories in the current path
        if (directory->d_type == DT_DIR || directory->d_type == DT_REG || directory->d_type == DT_LNK)
        {
            if (directory->d_name[0] == '.')
                continue;
            if (option_l == 1)
            {
                print_l(paths, dirstream, directory, option_i);
            }
            else if (option_l == 0 && option_i == 1)
            {
                printf("%lu ", directory->d_ino);
                printf("%s  ", directory->d_name);
            }
            else
            {
                printf("%s  ", directory->d_name);
            }
        }
    }
    printf("\n");
    if (!(dirstream = opendir(paths)))
        return;
    while ((directory = readdir(dirstream)) != NULL)
    {
        //the there is a directory, calls the function again with the new directory
        if (directory->d_type == DT_DIR)
        {
            char next[SIZE];
            if (directory->d_name[0] == '.')
            {
                continue;
            }
            snprintf(next, sizeof(next), "%s/%s", paths, directory->d_name);
            printf("\n%s:\n", next);
            print_R(next, directory, option_l, option_i);
        }
    }
}

int main(int argc, char **argv)
{
    char *dir_name;
    DIR *dirstream;
    struct dirent *directory;

    char absolute[SIZE];

    int allowed = 0;
    int option_l = 0;
    int option_i = 0;
    int option_R = 0;

    //if directory name is given, open that and make it the path
    if (argc == 3 && argv[2] != NULL)
    {
        dir_name = argv[2];
        if (!(dirstream = opendir(dir_name)))
        {
            perror("can not find directory");
            return -1;
        }
        //if absolute file path is given convert it to relative
        if (argv[2][0] == '\\' || argv[2][0] == '~'){
            char *convert = realpath(dir_name, absolute);
            dirstream = opendir(convert);
        }
        directory = readdir(dirstream);
    }
    else if (argc == 2 && argv[1][0] != '-')
    {
        dir_name = argv[1];
        if (!(dirstream = opendir(dir_name)))
        {
            perror("can not find directory");
            return -1;
        }
        directory = readdir(dirstream);
        allowed = 1;
    }
    //otherwise use root directory as the path
    else
    {
        dir_name = ".";
        dirstream = opendir(dir_name);
        directory = readdir(dirstream);
    }

    //if no options or paths are specified, print everything in current directory
    if (argc == 1 || allowed == 1)
    {
        for (; directory; directory = readdir(dirstream))
        {
            if (directory->d_name[0] == '.')
            {
                continue;
            }
            printf("%s  ", directory->d_name);
        }
        printf("\n");
    }

    //checks if there is l or i or R present in the ls command argument
    else if (argc > 1)
    {
        if (argv[1][0] == '-' && strchr(argv[1], 'l'))
        {
            option_l = 1;
        }
        if (argv[1][0] == '-' && strchr(argv[1], 'i'))
        {
            option_i = 1;
        }
        if (argv[1][0] == '-' && strchr(argv[1], 'R'))
        {
            option_R = 1;
        }

        //prints the output for the -l command
        if (option_l == 1 && option_R == 0)
        {
            print_l(dir_name, dirstream, directory, option_i);
        }

        //prints the output for the -i command
        else if (option_i == 1 && option_R == 0)
        {
            print_i(dirstream, directory);
        }

        //prints the output for the -R command
        else if (option_R == 1)
        {
            printf(".:\n");
            print_R(dir_name, directory, option_l, option_i);
        }
        else
        {
            printf("Command not found \n");
        }
    }
    closedir(dirstream);
}