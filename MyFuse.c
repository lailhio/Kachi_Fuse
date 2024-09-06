#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fuse.h>
#include "tree.h"
#include "list.h"
#include <unistd.h> 
#include <pwd.h>	// Due to getUsername()
#define MAX_NAMELEN 255
#define BLOCKSIZE (1024UL * 4)

struct kachi_entry {
    struct list_node node;
    char name[MAX_NAMELEN + 1];
    void *data;
    struct stat vstat;
};

static struct list_node entries;

char *getUsername() {
	register struct passwd *pw;
  	register uid_t uid;
  	uid = geteuid();
  	pw = getpwuid(uid);
	return pw->pw_name;
}

static int kachi_getattr(const char* path, struct stat* st)
{
    printf("%s: %s\n", __FUNCTION__, path); 
    struct list_node * p;
    struct kachi_entry * entry;
    memset(st,0,sizeof(struct stat));
    //If it is root dir
    if(!strcmp(path, "/")){
        st->st_mode= 0755 | S_IFDIR;
        st->st_size=0;
        st->st_nlink=2;

        list_for_each(p,&entries){
            entry=list_entry(p,struct kachi_entry,node);
            st->st_nlink++;
            st->st_size+=strlen(entry->name);
        }

        return 0;
    }
    else{
        list_for_each(p,&entries){
            entry=list_entry(p,struct kachi_entry,node);
            //Find the proper file
            if(!strcmp(path+1,entry->name)){
                *st = entry->vstat;
                return 0;
            }
        }
        return -ENOENT;
    }
}

static int kachi_mknode(const char *path, mode_t mode, dev_t rdev)
{
    printf("%s: %s\n", __FUNCTION__, path); 
    /*  1. Judge the name length
    2. Jugde whether the file exits*/
    struct kachi_entry* entry;
    struct list_node* p;
    //Name length
    if(strlen(path+1)>MAX_NAMELEN){
        return -ENAMETOOLONG;
    }
    //File Exits
    list_for_each(p,&entries){
        entry=list_entry(p,struct kachi_entry,node);
        if(!strcmp(entry->name,path+1)){
            return -EEXIST;
        }
    }
    entry=malloc(sizeof(struct kachi_entry));
    strcpy(entry->name,path+1);
    entry->vstat.st_mode=S_IFREG | mode;
    list_add_prev(&entry->node,&entries);
    return 0;
}
static int kachi_release(const char *path, struct fuse_file_info *fi)
{
    printf("%s: %s\n", __FUNCTION__, path);
    return 0;
}
static int kachi_unlink(const char* path)
{
    printf("%s: %s\n", __FUNCTION__, path); 
    struct list_node * p;
    struct kachi_entry * entry;
    //Find the file
    list_for_each(p,&entries){
        entry=list_entry(p,struct kachi_entry,node);
        if(!strcmp(entry->name,path+1)){
            list_del(p);
            free(entry);
            return 0;
        }
    }
    //Not found
    return -ENOENT;
}

static int kachi_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info* fi)
{
    printf("%s: %s\n", __FUNCTION__, path); 
    struct list_node * p;
    struct kachi_entry * entry;
    //Judge whether it is dir
    if(strcmp(path,"/")){
        return -EINVAL;
    }

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    //Find the dir
    list_for_each(p,&entries){
        entry=list_entry(p,struct kachi_entry,node);
        filler(buf, entry->name, NULL, 0);
    }
    //Not found
    return 0;
}
static int kachi_mkdir(const char* path, mode_t mode){
/*  1. Judge the name length
    2. Jugde whether the file exits*/
    printf("%s: %s\n", __FUNCTION__, path); 
    struct kachi_entry* entry;
    struct list_node* p;
    //Name length
    if(strlen(path+1)>MAX_NAMELEN){
        return -ENAMETOOLONG;
    }
    //File Exits
    list_for_each(p,&entries){
        entry=list_entry(p,struct kachi_entry,node);
        if(!strcmp(entry->name,path+1)){
            return -EEXIST;
        }
    }
    entry=malloc(sizeof(struct kachi_entry));
    strcpy(entry->name,path+1);
    entry->vstat.st_mode=S_IFDIR | mode;
    list_add_prev(&entry->node,&entries);
    return 0;
}
static int kachi_rmdir(const char* path){
    printf("%s: %s\n", __FUNCTION__, path); 
    struct list_node * p;
    struct kachi_entry * entry;
    //Judge whether it is dir
    //Find the dir
    list_for_each(p,&entries){
        entry=list_entry(p,struct kachi_entry,node);
        if(!strcmp(entry->name,path+1)){
            list_del(p);
            free(entry);
            return 0;
        }
    }
    //Not found
    return -ENOENT;

}

static int kachi_open(const char *path, struct fuse_file_info *fi)
{   
    printf("%s: %s\n", __FUNCTION__, path); 
    struct kachi_entry* entry;
    struct list_node* p;
 
    list_for_each(p,&entries){
        entry=list_entry(p,struct kachi_entry,node);
        //if find right file
        if(!strcmp(entry->name,path+1)){
            if (S_ISDIR(entry->vstat.st_mode)) {
                return -EISDIR;
            }
            fi->fh = (unsigned long)entry;
            return 0;
        }
    }
    //File not found
    entry=malloc(sizeof(struct kachi_entry));
    strcpy(entry->name,path+1);
    entry->vstat.st_mode=S_IFREG| 0755;
    list_add_prev(&entry->node,&entries);
    fi->fh = (unsigned long)entry;
    return 0;

}
static int kachi_write(const char* path, const char* buf, size_t bytes,
                    off_t offset, struct fuse_file_info* fi)
{
    printf("%s: %s, size: %zd\n", __FUNCTION__, path, bytes);
    struct kachi_entry* entry=(struct kachi_entry *)fi->fh;
    struct list_node* p;
    blkcnt_t req_blocks = (offset + bytes + BLOCKSIZE - 1) / BLOCKSIZE;
    void *newdata = realloc(entry->data, req_blocks * BLOCKSIZE);
    if (!newdata) { 
        return -ENOMEM;
    }
    entry->data = newdata;
    memcpy(entry->data + offset, buf, bytes);
    off_t minsize = offset + bytes;
    if (minsize > entry->vstat.st_size) {
        entry->vstat.st_size = minsize; 
    }
    return bytes;
}

static int kachi_read(const char* path, char* buf, size_t bytes, off_t offset,
                   struct fuse_file_info* fi)
{
    struct kachi_entry* entry=(struct kachi_entry *)fi->fh;
    printf("%s: %s\n", __FUNCTION__, path); 
    off_t filesize = entry->vstat.st_size; 
    if (offset > filesize) {
        return 0;
    }
    size_t avail = filesize - offset; 
    size_t rsize = (bytes < avail) ? bytes : avail;
    memcpy(buf, entry->data + offset, rsize);
    return rsize;
}
static int kachi_truncate(const char* path, off_t size){
    printf("%s: %s\n", __FUNCTION__, path);
    return 0;
}
static struct fuse_operations kachi_fs_ops = {
    .getattr    =   kachi_getattr,
    .mkdir      =   kachi_mkdir,
    .rmdir      =   kachi_rmdir,
    .readdir    =   kachi_readdir,
    .mknod      =   kachi_mknode,
    .open       =   kachi_open,
    .read       =   kachi_read,
    .write      =   kachi_write,
    .unlink     =   kachi_unlink,
    .release    =   kachi_release,
    .truncate   =   kachi_truncate,
};
// gcc MyFuse.c -o MyFuse `pkg-config fuse --cflags --libs`
int main(int argc, char* argv[])
{
    list_init(&entries);

    return fuse_main(argc, argv, &kachi_fs_ops, NULL);
}