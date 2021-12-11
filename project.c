#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/ktime.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/syscalls.h>

///< The license type -- this affects runtime behavior
MODULE_LICENSE("GPL");
  
///< The author -- visible when you use modinfo
MODULE_AUTHOR("SuperSalmonDong");
  
///< The description -- see modinfo
MODULE_DESCRIPTION("Thread API and Concurrency project");
  
///< The version of the module
MODULE_VERSION("1.0");

// *** Sorting ***
typedef struct Node node;
struct Node
{
    int pid;
    long int state;
    long long int running_time;
    long long int waiting_time;
    long long int process_time;
    long long int cpu_usage;
    char *command;

	node *next;
};

// Global Node
node *head;

void printList(node *head);
void push(node **head_ref,int pid,long int state,long long int running_time,long long int waiting_time,long long int process_time,long long int cpu_usage, char *command);
void insertionSort(node **head_ref);
void insertIntoSorted(node **sorted_ref, node *new_node);

void push(node **head_ref,int pid,long int state,long long int running_time,long long int waiting_time,long long int process_time,long long int cpu_usage, char *command)
{
	// allocate node and fill
	node *new_node;
	new_node = (node *)kmalloc(sizeof(node),GFP_KERNEL);
	new_node->pid = pid;
	new_node->cpu_usage = cpu_usage;
	new_node->state = state;
	new_node->running_time = running_time;
	new_node->waiting_time = waiting_time;
	new_node->process_time = process_time;
    new_node->command = command;

	// link
	new_node->next = *head_ref;
	*head_ref = new_node;
}

void printList(node *head)
{
	node *temp;
	temp = head;
	printk(KERN_INFO "\tpid\tstate\trunning_time\twaiting_time\tprocess_time\tcpu_usage(percent)\tcommand\n");
	while (temp != NULL)
	{
		//printf("%s\t%0.4f\n",temp->user, temp->cpu_usage);
		printk("\t%d\t%ld\t%-10lld\t%-10lld\t%-10lld\t%lld x 10^-4\t\t%s\n",temp->pid,temp->state,temp->running_time,temp->waiting_time,temp->process_time,temp->cpu_usage,temp->command);
		temp = temp->next;
	}
}

/* Function to sort a singly linked list using insertion sort */
void insertionSort(node **head_ref)
{
    /* Traverse the given linked list and insert every node to "sorted" */
    node *currentNode;
	/* Initialize the sorted linked list */
	node *sorted;
	sorted = NULL;

	currentNode = *head_ref;
	while (currentNode != NULL)
	{
		/* Store "next" for next iteration */
		node *next;
		next = currentNode->next;

		/*Insert "currentNode" into the "sorted" linked list */
		insertIntoSorted(&sorted, currentNode);

		/* Update "currentNode" to the next node */
		currentNode = next;
	}
	*head_ref = sorted;
}
/* Function to insert a given node in the "sorted" linked list. Where
 * the insertion sort actually occurs.
 */
void insertIntoSorted(node **sorted_ref, node *new_node)
{
	node *currentNode;
	/* Special case for the head end of the "sorted" */
	if ((*sorted_ref == NULL) || ((*sorted_ref)->cpu_usage <= new_node->cpu_usage))
	{
		new_node->next = *sorted_ref;
		*sorted_ref = new_node;
	}
	/* Locate the node before the point of insertion */
	else
	{
		currentNode = *sorted_ref;
		while ((currentNode->next != NULL) && (currentNode->next->cpu_usage > new_node->cpu_usage))
		{
			currentNode = currentNode->next;
		}
		new_node->next = currentNode->next;
		currentNode->next = new_node;
	}
}
  
// Main Function  
SYSCALL_DEFINE0(project)
{
    struct task_struct *task;
    head = NULL;
    for_each_process(task)
    {
        long long int process_time;
        long long int waiting_time;
        long long int cpu_usage;
        s64  uptime;
        long long int running_time;
        long starttime = (task->start_time) / 1000000; // from nano second to ms

        running_time = ((task->utime) +  (task->stime)) / 1000000; // from nano second to ms

        uptime = ktime_to_ms(ktime_get_boottime()); // In ms

        process_time = uptime - ((long long int)starttime); // Total Process time in ms

		waiting_time = process_time - running_time; // waiting time in ms
        
        cpu_usage = (running_time * 1000000) / process_time; // x10^-4 percent

        // push to linked list
        push(&head,task->pid,task->__state,running_time,waiting_time,process_time,cpu_usage,task->comm);
    }
    insertionSort(&head); // sort
    printList(head); // print

    return 0;
}
  
