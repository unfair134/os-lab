// COMP3511 Spring 2024
// PA2: Completely Fair Scheduler
//
// Your name:Qiu Tian
// Your ITSC email:  tqiuae@connect.ust.hk
//
// Declaration:
//
// I declare that I am not involved in plagiarism
// I understand that both parties (i.e., students providing the codes and students copying the codes) will receive 0 marks.


// =======================================================================
// Note: You can add extra variables and implement extra helper functions
// =======================================================================

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


// ===
// Region: Constants
// ===

// vruntime is a floating-point number,
// There may be some minor precision errors which are hard to grade
// The DISPLAY_VRUNTIME should be turned off for the final output
// During development, you can turn on the debug mode to show the vruntime
#define DISPLAY_VRUNTIME 0

// Define MAX_PROCESS
// For simplicity, assume that we have at most 10 processes
#define MAX_PROCESS 10

// Assume that we only need to support 2 types of space characters:
// " " (space), "\t" (tab)
#define SPACE_CHARS " \t"

// Keywords (to be used when parsing the input)
#define KEYWORD_NUM_PROCESS "num_process"
#define KEYWORD_SCHED_LATENCY "sched_latency"
#define KEYWORD_MIN_GRANULARITY "min_granularity"
#define KEYWORD_BURST_TIME "burst_time"
#define KEYWORD_NICE_VALUE "nice_value"

// Useful string template used in printf()
// We will use diff program to auto-grade
// Please use the following templates in printf to avoid formatting errors
//
// Examples:
//   printf(template_cfs_algorithm)  # print === CFS algorithm ===\n on the screen
//   printf(template_step_i, 0)      # print === Step 0 ===\n on the screen

const char template_key_value_pair[] = "%s = %d\n";
const char template_parsed_values[] = "=== CFS input values ===\n";
const char template_cfs_algorithm[] = "=== CFS algorithm ===\n";
const char template_step_i[] = "=== Step %d ===\n";
const char template_gantt_chart[] = "=== Gantt chart ===\n";

// The lookup table for the mapping of nice value to the weight value
static const int DEFAULT_WEIGHT = 1024;
static const int NICE_TO_WEIGHT[40] = {
    88761, 71755, 56483, 46273, 36291, // nice: -20 to -16
    29154, 23254, 18705, 14949, 11916, // nice: -15 to -11
    9548, 7620, 6100, 4904, 3906,      // nice: -10 to  -6
    3121, 2501, 1991, 1586, 1277,      // nice:  -5 to  -1
    1024, 820, 655, 526, 423,          // nice:   0 to   4
    335, 272, 215, 172, 137,           // nice:   5 to   9
    110, 87, 70, 56, 45,               // nice:  10 to  14
    36, 29, 23, 18, 15,                // nice:  15 to  19
};

// struct CFSProcess - a data structure for the CFS scheduling
struct CFSProcess
{
    int weight;      // the weight value, lookup based on the nice value
    double vruntime; // each process needs to store its vruntime
    int remain_time; // initially, it is equal to the burst_time. This value keeps decreasing until 0
    int time_slice;  // to be calculated based on the input values at the beginning of the CFS scheduling
};

// Constant and data structure for Gantt chart dispaly
#define MAX_GANTT_CHART 300
struct GanttChartItem
{
    int pid;
    int duration;
};

// ===
// Region: Global variables:
// For simplicity, let's make everything static without any dyanmic memory allocation
// In other words, we don't need to use malloc()/free()
// It will save you lots of time to debug if everything is static
// ===
int num_process = 0;
int sched_latency = 0;
int min_granularity = 0;
int burst_time[MAX_PROCESS]; 
int nice_value[MAX_PROCESS]; 
struct CFSProcess process[MAX_PROCESS];
struct GanttChartItem chart[MAX_GANTT_CHART];
int num_chart_item = 0;
int finish_process_count = 0; // the number of finished process
int weight_sum = 0;
// ===
// Region: Functions
// ===


void initialize_cfs_process()
{
    // TODO: initialize CFS process table
    //calculate sum
    for (size_t i = 0; i < num_process; i++)
    {
        weight_sum+=NICE_TO_WEIGHT[nice_value[i]+20];
    }
    
    for (int i=0;i<num_process;i++){
        struct CFSProcess p={
            NICE_TO_WEIGHT[nice_value[i]+20],
            0.0,
            burst_time[i],
            NICE_TO_WEIGHT[nice_value[i]+20]*sched_latency/weight_sum
        };
        process[i]=p;
    }
    //print_parsed_values();
}
int search_min_vruntime(){
    int pid=-1;
    double min_vt=999.9;
    for (size_t i = 0; i < num_process; i++)
    {
        if (process[i].vruntime<min_vt && process[i].remain_time>0)
        {
            pid=i;
            min_vt=process[i].vruntime;  
        }
    }
    return pid;
}

int check_grand(int timeslice){
    if (timeslice>=min_granularity)
    {
        return timeslice;
    }
    return min_granularity;
}
void execute_cfs_scheduling()
{   int step=0;
    printf(template_cfs_algorithm);
    printf(template_step_i,step);
    print_cfs_process();
    // TODO: write your CFS algorithm code
    while (finish_process_count<num_process)
    {   
        /* code */
        int pid_now = search_min_vruntime();
        int runtime=0;
        runtime=check_grand(process[pid_now].time_slice);
        if(runtime>=process[pid_now].remain_time){
            runtime=process[pid_now].remain_time;
            //process[pid_now].remain_time=0;
            finish_process_count++;
        }
        process[pid_now].remain_time-=runtime;
        process[pid_now].vruntime+=DEFAULT_WEIGHT/(double)process[pid_now].weight*(double)runtime;

        step++;
        printf(template_step_i,step);
        print_cfs_process();
        
        if(num_chart_item>0&&chart[num_chart_item-1].pid==pid_now){
            chart[num_chart_item-1].duration+=runtime;
        }
        else{
        struct GanttChartItem gantt={pid_now,runtime};
        chart[num_chart_item]=gantt;
        num_chart_item++;}
    }

    //gantt_chart_print();
}

// ===
// All functions below are given
// You don't need to make further changes on them
// ===

void gantt_chart_print()
{
    int t = 0, i = 0, id = 0;
    printf(template_gantt_chart);
    if (num_chart_item > 0)
    {
        printf("%d ", t);
        for (i = 0; i < num_chart_item; i++)
        {
            t = t + chart[i].duration;
            printf("P%d %d ", chart[i].pid, t);
        }
        printf("\n");
    }
    else
    {
        printf("The gantt chart is empty\n");
    }
}

// Helper function: Check whether the line is a blank line (for input parsing)
// This function is only used in the input parsing
int is_blank(char *line)
{
    char *ch = line;
    while (*ch != '\0')
    {
        if (!isspace(*ch))
            return 0;
        ch++;
    }
    return 1;
}
// Helper function: Check whether the input line should be skipped
// This function is only used in the input parsing
int is_skip(char *line)
{
    if (is_blank(line))
        return 1;
    char *ch = line;
    while (*ch != '\0')
    {
        if (!isspace(*ch) && *ch == '#')
            return 1;
        ch++;
    }
    return 0;
}

// This function is only used in the input parsing
void parse_arguments(char **argv, char *line, int *numTokens, char *delimiter)
{
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
}

void parse_input()
{
    FILE *fp = stdin;
    char *line = NULL;
    ssize_t nread;
    size_t len = 0;

    char *two_tokens[2];               // buffer for 2 tokens
    char *process_tokens[MAX_PROCESS]; // buffer for n tokens
    int numTokens = 0, n = 0, i = 0;
    char equal_plus_spaces_delimiters[5] = "";

    strcpy(equal_plus_spaces_delimiters, "=");
    strcat(equal_plus_spaces_delimiters, SPACE_CHARS);

    while ((nread = getline(&line, &len, fp)) != -1)
    {
        if (is_skip(line) == 0)
        {
            line = strtok(line, "\n");
            if (strstr(line, KEYWORD_NUM_PROCESS))
            {
                // parse num_process
                parse_arguments(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2)
                {
                    sscanf(two_tokens[1], "%d", &num_process);
                }
            }
            else if (strstr(line, KEYWORD_SCHED_LATENCY))
            {
                // parse sched_latency
                parse_arguments(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2)
                {
                    sscanf(two_tokens[1], "%d", &sched_latency);
                }
            }
            else if (strstr(line, KEYWORD_MIN_GRANULARITY))
            {
                // parse min_granularity
                parse_arguments(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2)
                {
                    sscanf(two_tokens[1], "%d", &min_granularity);
                }
            }
            else if (strstr(line, KEYWORD_BURST_TIME))
            {

                // parse the burst_time
                // note: we parse the equal delimiter first
                parse_arguments(two_tokens, line, &numTokens, "=");
                if (numTokens == 2)
                {
                    // parse the second part using SPACE_CHARS
                    parse_arguments(process_tokens, two_tokens[1], &n, SPACE_CHARS);
                    for (i = 0; i < n; i++)
                    {
                        sscanf(process_tokens[i], "%d", &burst_time[i]);
                    }
                }
            }
            else if (strstr(line, KEYWORD_NICE_VALUE))
            {
                // parse the nice_value
                // note: we parse the equal delimiter first
                parse_arguments(two_tokens, line, &numTokens, "=");
                if (numTokens == 2)
                {
                    // parse the second part using SPACE_CHARS
                    parse_arguments(process_tokens, two_tokens[1], &n, SPACE_CHARS);
                    for (i = 0; i < n; i++)
                    {
                        sscanf(process_tokens[i], "%d", &nice_value[i]);
                    }
                }
            }
        }
    }
}

// Helper function: print an array of integer
// This function is used in print_parsed_values
void print_int_array(char *name, int vec[MAX_PROCESS], int n)
{
    int i;
    printf("%s = [", name);
    for (i = 0; i < n; i++)
    {
        printf("%d", vec[i]);
        if (i < n - 1)
            printf(",");
    }
    printf("]\n");
}

void print_parsed_values()
{
    printf(template_parsed_values);
    printf(template_key_value_pair, KEYWORD_NUM_PROCESS, num_process);
    printf(template_key_value_pair, KEYWORD_SCHED_LATENCY, sched_latency);
    printf(template_key_value_pair, KEYWORD_MIN_GRANULARITY, min_granularity);
    print_int_array(KEYWORD_BURST_TIME, burst_time, num_process);
    print_int_array(KEYWORD_NICE_VALUE, nice_value, num_process);
}

void print_cfs_process()
{
    int i;
    if (DISPLAY_VRUNTIME)
    {
        // Print out an extra vruntime for debugging
        printf("Process\tWeight\tRemain\tSlice\tvruntime\n");
        for (i = 0; i < num_process; i++)
        {
            printf("P%d\t%d\t%d\t%d\t%.2f\n",
                   i,
                   process[i].weight,
                   process[i].remain_time,
                   process[i].time_slice,
                   process[i].vruntime);
        }
    }
    else
    {
        printf("Process\tWeight\tRemain\tSlice\n");
        for (i = 0; i < num_process; i++)
        {
            printf("P%d\t%d\t%d\t%d\n",
                   i,
                   process[i].weight,
                   process[i].remain_time,
                   process[i].time_slice);
        }
    }
}


// The main function
int main()
{
    parse_input();
    print_parsed_values();
    initialize_cfs_process();
    execute_cfs_scheduling();
    gantt_chart_print();
    return 0;
}
