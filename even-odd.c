#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>

int count_stdin(FILE *fp);

int fill_nums(int *numbas, FILE *fp);

void switchem(int pindx, int eo_flag, int size, float psize, int start, int end, int *numbas, int *active);

void synch(int par_id,int par_count, int *flags);
//int startednow = 0;

int main(int argc, char *argv[])
{
    clock_t cstartc;
    cstartc = clock();

    FILE *fp = stdin;
    int i_count = count_stdin(fp);
    int p_count = atoi(argv[1]);
    if(p_count < 1)
    {
        return 0;
    }
    

    rewind(fp);

    int *nums = (int *)mmap(NULL, i_count * sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    int *flags = (int *)mmap(NULL, (50 + p_count) * sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    int *active = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    int n_count = fill_nums(nums, fp);

    if( (p_count * 2) > n_count) { p_count = n_count/2; }

    fprintf(stderr, "Size: %d | Procces Count: %d \n", n_count, p_count);
    fprintf(stderr, "Initial: [");
    for(int i = 0; i < n_count; i ++)
    {
        if(i+1<n_count) { fprintf(stderr, "%d, ", nums[i]); }
        else { fprintf(stderr, "%d]", nums[i]); }
    }
    fprintf(stderr, "\n");

    ////forkkkkk

    pid_t ppids[p_count];

    for(int i = 0; i < p_count; i++)
    {
        pid_t fok = fork();
        if(fok == 0)
        {
            sleep(2);

            int index = i;
            float size = n_count;
            float psize = p_count;
            float increment = size/psize;
            int even_odd_flag = 0;
            int start = 0;
            int end = 0;
            float start_temp =  index * increment;
            float end_temp = (index + 1) * increment;
            while( (start+2) <= start_temp)
            {
                start += 2;
            }
            while( (end+2) <= end_temp)
            {
                end += 2;
            }
            //fprintf(stderr, "IDX: %d| start %d | end: %d  \n", index, start, end);

            while(1)
            {
                
                //fprintf(stderr, "IDX: %d at top   \n", index);

                synch(index, p_count, flags);
                    
                *active = 0;
                switchem(index, even_odd_flag, (int) size, psize, start, end, nums, active);

                even_odd_flag = 1;
                synch(index, p_count, flags);

                switchem(index, even_odd_flag, (int) size, psize, start, end, nums, active);

                even_odd_flag = 0;
                synch(index, p_count, flags);
        
                // if(index == 0)
                // {
                //     fprintf(stderr, "this step: [");
                //     for(int i = 0; i < n_count; i ++)
                //     {
                //         if(i+1<n_count) { fprintf(stderr, "%d, ", nums[i]); }
                //         else { fprintf(stderr, "%d]", nums[i]); }
                //     }
                //     fprintf(stderr, "\n");
                // }

                if( *active != 1 )
                {
                    break;
                }
            }
            return 0;
        }
        else
        {
            ppids[i] = fok;
        }
    }

    for(int i = 0; i < p_count; i++)
    {
        waitpid(ppids[i], NULL, NULL);
    }

    

    fprintf(stderr, "Final: [");
    for(int i = 0; i < n_count; i ++)
    {
        if(i+1<n_count) { fprintf(stderr, "%d, ", nums[i]); }
        else { fprintf(stderr, "%d]", nums[i]); }
    }
    fprintf(stderr, "\n");

    munmap(nums, i_count * sizeof(int));
    munmap(flags, (50 + p_count) * sizeof(int));
    munmap(active, sizeof(int));

    clock_t cendc;
    cendc = clock();
    clock_t elapsed = (cendc - cstartc);
    fprintf(stderr, "Time Elapsed: %ld \n", elapsed);
    
    return 0;
}

void switchem(int pindx, int eo_flag, int size, float psize, int start, int end, int *numbas, int *active)
{
    if(eo_flag == 1)
    {
        start += 1;
        end += 1;
    }

    if(end > size)
    {
        end = size;
        if(end <= start)
        {
            fprintf(stderr, "HUHHHHH \n");
            return;
        }
    }

    int b;

    while( ( start + 1 ) < end )
    {
        b = start + 1;
        if( numbas[start] > numbas[b] )
        {
            int c = numbas[start];
            numbas[start] = numbas[b];
            numbas[b] = c;
            *active = 1;
        }
        start += 2;
    }

}

void synch(int par_id,int par_count, int *flags)
{
    int idl = 0;
    int f1 = par_count+1;
    int f2 = par_count+2;
    
    flags[par_id] = 1;

    int ccccc = 0;
    
    while(1)
    {
        if(ccccc > 100000)
        {
            ccccc = 0;
            //fprintf(stderr, "idx: %d stuck 1 \n", par_id);

        }
        ccccc += 1;


        idl = 0;
        for(int i = 0; i < par_count; i++)
        {
            if(flags[i] != 1)
            {
                idl += 1;
            }
        }
        if(idl == 0 || flags[f1] == 1)
        {
            flags[f1] = 1;   
            flags[par_id] = 0;
            break;
        }
        else
        {
            //sleep(1);
        }
        
    }
    while(1)
    {
        if(ccccc > 100000)
        {
            ccccc = 0;
            //fprintf(stderr, "idx: %d stuck 2 \n", par_id);

        }
        ccccc += 1;

        idl = 0;
        for(int i = 0; i < par_count; i++)
        {
            if(flags[i] == 1)
            {
                idl += 1;
            }
        }
        if(idl == 0 || flags[f1] == 0)
        {
            flags[f1] = 0;
            break;
        }
    }
}

int count_stdin(FILE *fp)
{
    int coun = 0;
    int start_flag = 0;
    int askey;
    char c[2];

    while(fgets(c, sizeof(c), fp) != NULL)
    {
        askey = (int) c[0];
        if(askey == 32 || askey == 10)
        {
            start_flag = 0;
        }
        else
        {
            if(start_flag == 0)
            {
                coun += 1;
            }
            start_flag = 1;
        }
    }

    return coun;
}

int fill_nums(int *numbas, FILE *fp)
{
    int current = 0;
    int negative_flag = 0;
    int count = 0;
    int start_flag = 0;
    int askey;
    char c[2];

    while(fgets(c, sizeof(c), fp) != NULL)
    {
        askey = (int) c[0];
        if(askey == 32)
        {
            if(start_flag == 1)
            {
                if(negative_flag == 1)
                {
                    current = -current;
                }
                numbas[count-1] = current;
                start_flag = 0;
                current = 0;
                negative_flag = 0;
            }
        }
        else if(askey == 45)
        {
            negative_flag = 1;
        }
        else if(askey != 10)
        {
            if(start_flag == 0)
            {
                count += 1;
            }
            start_flag = 1;
            int cc = atoi(c);
            current = 10*current + cc;
        }
    }
    if(askey != 32 && count > 0 && askey != 10)
    {
        numbas[count-1] = current;
    }
    return count;
}