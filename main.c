#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

void swap (int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

void shuffle ( int arr[], int n, int seed )
{
    srandom ( seed );
    int i;
    for (i = n-1; i > 0; i--)
    {
        int j = random() % (i+1);

        swap(&arr[i], &arr[j]);
    }
}

int main( int argc, char *argv[] )
{
    MPI_File fh;
    int buf[1000], my_rank,p,voters,candidates,i,j,z,portionsize,newcandidate,readvoters;
    int *arrcandidate;
    MPI_Status status;

    MPI_Init( &argc, &argv );
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    MPI_File_open(MPI_COMM_WORLD, "project.txt",MPI_MODE_CREATE|MPI_MODE_RDWR,MPI_INFO_NULL, &fh);

    if (my_rank == 0)
    {

        printf("please enter number of candidates : ");
        scanf("%d",&candidates);

        printf("please enter number of voters : ");
        scanf("%d",&voters);

        MPI_File_seek( fh, 0, MPI_SEEK_SET );
        MPI_File_write(fh,&candidates, 1, MPI_INT, &status);
        MPI_File_write(fh,&voters, 1, MPI_INT, &status);
        printf("check your file \n");


        int *candidatesList;
        candidatesList = (int *) malloc(candidates * sizeof(int));

        for (i = 0; i < candidates; ++i)
        {
            candidatesList[i] = i + 1;
        }
        int seed=0;
        for (i = 0; i < voters; ++i)
        {
            shuffle(candidatesList, candidates, seed++);
            for(j = 0; j < candidates; ++j)
            {
                MPI_File_write(fh,&candidatesList[j], 1, MPI_INT, &status);

            }
        }
        /*
        for(j=0;j<voters;j++){
            for( i = 0 ; i < candidates ; i++ ) {
                arr[i]=-1;
            }
            for(i=0;arr[candidates-1] == -1;i++){
                int x =1+rand()%(candidates);
                int counter=0;
                for(z=0;z<i ;z++){
                    if(arr[z]==x ){
                        counter++;
                    }
                }if(counter>0){
                    i--;
                    continue;
                }
                arr[i]=x;
                MPI_File_write(fh,&arr[i], 1, MPI_INT, &status);
            }
        }*/

        MPI_File_seek( fh, 0, MPI_SEEK_SET );
        MPI_File_read( fh, &newcandidate, 1, MPI_INT, &status );

        MPI_File_read( fh, &readvoters, 1, MPI_INT, &status );
        int all=candidates*voters;
        int x=0;
        for(i=0; i<voters; i++)
        {
            for(j=0; j<candidates; j++)
            {
                MPI_File_read( fh, &x, 1, MPI_INT, &status );
                printf(" %d ",x);
            }
            printf("\n");
        }

    }
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_File_seek( fh, 0, MPI_SEEK_SET );
    MPI_File_read( fh, &newcandidate, 1, MPI_INT, &status );

    int *scores = (int *) malloc(newcandidate * sizeof(int));
    int *totalScores = (int *) malloc(newcandidate * sizeof(int));

    MPI_File_read( fh, &readvoters, 1, MPI_INT, &status );
    int smalport=readvoters/p;
    arrcandidate = malloc(smalport*newcandidate*sizeof(int));

    for (i = 0; i < newcandidate; ++i)
        scores[i] = 0;

    int offset=(2*sizeof(int))+(smalport*newcandidate*(my_rank)*sizeof(int));
    MPI_File_seek( fh,offset, MPI_SEEK_SET );
    for(i=0; i<smalport; i++)
    {
        int x=0;
        for(j=0; j<newcandidate; j++)
        {
            MPI_File_read( fh, &x, 1, MPI_INT, &status );
            arrcandidate[(i*newcandidate)+j]=x;

        }
    }
    for(i=0; i<smalport; i++)
    {
        int e = arrcandidate[i*newcandidate] - 1;
        scores[e]++;
    }
    /*printf("rank = %d scores: ", my_rank);
    for (i = 0; i < newcandidate; ++i)
        printf("%d ", scores[i]);
    printf("\n");
    */
    MPI_Reduce(scores, totalScores, newcandidate, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    /*if(my_rank == 0)
    {
        printf("scores: ");
        for (i = 0; i < newcandidate; ++i)
            printf("%d ", totalScores[i]);
        printf("\n");

    }*/
    int maxi1 = 0, maxIndex1 = 0;
    int maxi2 = 0, maxIndex2 = 1;
    int percentage, flag = 0;
    if( my_rank == 0)
    {
        printf("Round 1 results:\n");
        for( i = 0; i < candidates; ++i)
        {
            percentage = (double) totalScores[i] / voters * 100;
            printf("Candidate %d got %d out of %d votes, which is %d%% \n", i+1, totalScores[i], voters, percentage);
            if(totalScores[i] > maxi1)
            {
                maxi1 = totalScores[i];
                maxIndex1 = i;
            }
            else if(totalScores[i] > maxi2)
            {
                maxi2 = totalScores[i];
                maxIndex2 = i;
            }

        }
        percentage = (double) maxi1 / voters * 100;
        if(percentage > 50)
        {
            printf("Candidate %d gets %d votes and wins in round 1 \n", maxIndex1+1, maxi1);
            flag = 1;
        }
    }
    MPI_Bcast(&flag, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (flag == 1)
    {
        MPI_Finalize();
        return 0;
    }
    MPI_Bcast(&maxIndex1, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&maxIndex2, 1, MPI_INT, 0, MPI_COMM_WORLD);
    //printf("rank = %d, candidate1 index = %d, candidate2 index = %d\n", my_rank, maxIndex1, maxIndex2);

    for (i = 0; i < newcandidate; ++i)
        scores[i] = 0;

    for(i = 0; i < smalport; i++)
    {
        for(j = 0; j < newcandidate; j++)
        {
            int e = arrcandidate[i*newcandidate+j] - 1;
            if (e == maxIndex1 || e == maxIndex2)
            {
                scores[e]++;
                break;
            }
        }
    }
    /*printf("rank = %d scores: ", my_rank);
    for (i = 0; i < newcandidate; ++i)
        printf("%d ", scores[i]);
    printf("\n");
    */
    MPI_Reduce(scores, totalScores, newcandidate, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if( my_rank == 0)
    {
        printf("Round 2 results:\n");
        maxi1 = totalScores[maxIndex1];
        maxi2 = totalScores[maxIndex2];
        int percentage1 = (double) maxi1 / voters * 100;
        int percentage2 = (double) maxi2 / voters * 100;
        printf("Candidate %d got %d out of %d votes, which is %d%% \n", maxIndex1+1, maxi1, voters, percentage1);
        printf("Candidate %d got %d out of %d votes, which is %d%% \n", maxIndex2+1, maxi2, voters, percentage2);
        if(percentage1 > 50)
        {
            printf("Candidate %d gets %d votes and wins in round 2 \n", maxIndex1+1, maxi1);
        }
        else if(percentage2 > 50)
        {
            printf("Candidate %d gets %d votes and wins in round 2 \n", maxIndex2+1, maxi2);
        }
        else
        {
            printf("Draw! No one wins :(");
        }
    }

    MPI_Finalize();
    return 0;
}
