
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>

#include <sys/mount.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

typedef uint8_t 		u8;
typedef uint16_t 		u16;
typedef uint32_t 		u32;
typedef uint64_t 		u64;

extern u32 __setuid[];

int GetPid( const char* partOfCmd )
{
    int ret = 0;
    int i;

    char buf[ 0x100 ];

    for( i = 0; i < 0x7CFF; i++ )
    {
        snprintf( buf, sizeof( buf ), "/proc/%d/cmdline", i );
        int f = open( buf, O_RDONLY );
        if( f < 0 )
        {
            continue;
        }
        memset( buf, 0, sizeof( buf ) );
        read( f, buf, sizeof( buf ) );
        int success = 0;
        if( strstr( buf, partOfCmd ) )
        {
            success = 1;
        }

        close( f );
        if( success )
        {
            ret = i;
            break;
        }
    }


    return ret;
}

int PatchAdbd()
{
    struct stat st;
    int ret = -1;
    char *buf = NULL, *start = NULL, *end = NULL;

    u32 patch[] =
    {
        0xe3a00000,
        0
    };

    u32 *sgid = (u32*)&setgid;
    u32 *caps = (u32*)&capset;

    int fd = open( "/sbin/adbd", O_RDWR );
    if( fd < 0 )
    {
        perror( "open adbd for patching" );
        return -1;
    }

    if( fstat( fd, &st ) )
    {
        perror( "fstat adbd for patching" );
        goto out;
    }

    if( !(buf = memalign( 32, st.st_size ) ) )
    {
        perror( "malloc for adbd patch" );
        goto out;
    }

    if( read( fd, buf, st.st_size ) != st.st_size )
    {
        perror( "read for adbd patch" );
        goto out;
    }

    if( lseek64( fd, 0, SEEK_SET ) )
    {
        perror( "seek for adbd patch" );
        goto out;
    }

    for( start = buf, end = start + st.st_size - 0x20; start < end; start++ )
    {
        if( !memcmp( &start[1], &caps[1], sizeof( u32 ) * 2 ) )
        {
            printf( "found capset\n" );
            memcpy( &start[1], patch, sizeof( patch ) );
            start += sizeof( patch );
            continue;
        }
        if( !memcmp( &start[1], &__setuid[1], sizeof( u32 ) * 2 ) )
        {
            printf( "found setuid\n" );
            memcpy( &start[1], patch, sizeof( patch ) );
            start += sizeof( patch );
            continue;
        }
        if( !memcmp( &start[1], &sgid[1], sizeof( u32 ) * 2 ) )
        {
            printf( "found setgid\n" );
            memcpy( &start[1], patch, sizeof( patch ) );
            start += sizeof( patch );
            continue;
        }

        // galaxy s4 has some silliness where running the normal sh as root wont work
        // but running it as another name does work.
        if( !strcmp( start, "/system/bin/sh" ) )
        {
            strcpy( start, "/shell" );
            start += sizeof( "/system/bin/sh" );
        }
    }

    if( write( fd, buf, st.st_size ) != st.st_size )
    {
        perror( "write for adbd patch" );
        goto out;
    }

    fsync( fd );
    ret = 0;
    printf( "Patched adbd\n" );
out:
    free( buf );
    if( fd >= 0 )
    {
        close( fd );
    }

    return ret;
}

int RemountPartition( const char* partition, int option )
{
    FILE *fi = fopen( "/proc/mounts", "r" );
    if( !fi )
    {
        printf( "cant open /proc/mounts\n" );
        return -1;
    }
    char line[ 1024 ];
    char pCopy[ 0x20 ];
    int pCopySize = snprintf( pCopy, sizeof( pCopy ), " %s ", partition );

    char* col1 = line;
    char* col2, *col3, *col4;
    int found = 0;
    while( fgets( line, sizeof( line ), fi ) )
    {
        if( !( col2 = strchr( col1, ' ' ) )
                || !( col3 = strchr( col2 + 1, ' ' ) )
                || !( col4 = strchr( col3 + 1, ' ' ) ) )
        {
            continue;
        }
        if( strncmp( col2, pCopy, pCopySize ) )
        {
            continue;
        }
        *col2++ = 0;
        *col3++ = 0;
        *col4 = 0;
        found = 1;
        break;
    }
    fclose( fi );

    if( !found )
    {
        printf( "failed to parse /proc/mounts\n" );
        return -3;
    }

    // remount read/write
    if( mount( col1, col2, col3, MS_REMOUNT | option, NULL ) < 0 )
    {
        printf( "remount %s failed: %s\n", partition, strerror( errno ) );
        return -4;
    }

    return 0;
}

int CopyFile( const char* src, const char* dst, u32 mode )
{
    int ret = -1;
    int inFd = -1;
    int outFd = -1;

    struct stat st;

    if( ( inFd = open( src, O_RDONLY ) ) < 0 )
    {
        printf( "open %s: %s\n", src, strerror( errno ) );
        goto out;
    }

    if( ( outFd = open( dst, O_WRONLY | O_CREAT | O_TRUNC, 0600 ) ) < 0 )
    {
        printf( "open %s: %s\n", dst, strerror( errno ) );
        goto out;
    }

    if( fstat( inFd, &st ) )
    {
        printf( "fstat %s: %s\n", dst, strerror( errno ) );
        goto out;
    }

    if( sendfile( outFd, inFd, NULL, st.st_size ) != st.st_size )
    {
        printf( "sendfile %s -> %s: %s\n", src, dst, strerror( errno ) );
        goto out;
    }

    if( fchmod( outFd, mode ) )
    {
        printf( "fchmod %s: %s\n", dst, strerror( errno ) );
        goto out;
    }

    fsync( outFd );


    //printf( sendFileOk, src, dst );
    ret = 0;

out:
    if( inFd >= 0 )
    {
        close( inFd );
    }
    if( outFd >= 0 )
    {
        close( outFd );
    }
    if( ret && outFd > -1 )
    {
        unlink( dst );
    }
    return ret;
}

int main( int argc, char *argv[] )
{
    int ret = 0;
    // mount "/" to read-write
    if( RemountPartition( "/", 0 )

            // we kill adbd, because it cant be running when we patch it
            //! it makes it a bit hard to get all these useful printf(), but it has to be done
            || kill( GetPid( "adbd" ), 9 )

            // this is for the galaxy s4
            //! the noobs at Samsung block running "sh" as root, but block it only by name.
            //! it works fine if we copy sh to /shell.
            || CopyFile( "/system/bin/sh", "/shell", 0755 )

            // remove capset(2), setuid(2), and setgid(2) from adbd
            //! when it restarts, it will try to drop to shell user, but continue to run as root
            || PatchAdbd() )
    {
        ret = -1;
    }


    // remount "/" read-only
    RemountPartition( "/", MS_RDONLY );

    return ret;
}
