#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define main original_main
#include "../Q5_A_LL.c"
#undef main

int tests_run = 0;
int tests_passed = 0;
void check(int condition, const char *test_name) {
    tests_run++;
    if (condition) {
        tests_passed++;
        printf("[PASS] %s\n", test_name);
    } else {
        printf("[FAIL] %s\n", test_name);
    }
}

// Safely walk n nodes from head; returns NULL instead of crashing
// if the list is shorter than expected.
ListNode* nth(ListNode *head, int n) {
    while (head != NULL && n-- > 0) head = head->next;
    return head;
}

// Runs fn(fd) in a child process, so a crash inside the function under test
// (e.g. on an empty-list edge case) shows up as a failed check instead of
// taking the whole test binary down. Returns 1 if the child crashed.
int run_isolated(void (*fn)(int), int *out, int out_count) {
    int pipefd[2];
    pipe(pipefd);
    pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[0]);
        fn(pipefd[1]);
        close(pipefd[1]);
        _exit(0);
    }
    close(pipefd[1]);
    int status;
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status)) {
        close(pipefd[0]);
        return 1;
    }
    read(pipefd[0], out, sizeof(int) * out_count);
    close(pipefd[0]);
    return 0;
}

void build_list(LinkedList *ll, int *arr, int n) {
    ListNode *tail = NULL;
    ll->head = NULL;
    ll->size = 0;
    for (int i = 0; i < n; i++) {
        ListNode *node = malloc(sizeof(ListNode));
        node->item = arr[i];
        node->next = NULL;
        if (tail == NULL) {
            ll->head = node;
        } else {
            tail->next = node;
        }
        tail = node;
        ll->size++;
    }
}

void empty_list(LinkedList *ll) {
    ListNode *current = ll->head;
    while (current != NULL) {
        ListNode *next = current->next;
        free(current);
        current = next;
    }
    ll->head = NULL;
    ll->size = 0;
}

// out layout: [front.size, back.size, front.head != NULL ? 1 : 0, back.head != NULL ? 1 : 0]
void call_empty_list(int fd) {
    LinkedList ll, front, back;
    ll.head = NULL;
    ll.size = 0;
    front.head = NULL;
    front.size = 0;
    back.head = NULL;
    back.size = 0;
    frontBackSplitLinkedList(&ll, &front, &back);
    int out[4] = {front.size, back.size,
                  front.head != NULL ? 1 : 0, back.head != NULL ? 1 : 0};
    write(fd, out, sizeof(out));
}


int main(void) {
    LinkedList ll, front, back;

    // Case 1: Even length list splits evenly
    int a1[] = {1, 2, 3, 4, 5, 6};
    build_list(&ll, a1, 6);
    front.head = NULL;
    front.size = 0;
    back.head = NULL;
    back.size = 0;
    frontBackSplitLinkedList(&ll, &front, &back);
    check(front.size == 3, "Even length: front size is 3");
    check(back.size == 3, "Even length: back size is 3");
    check(nth(front.head, 0) != NULL && nth(front.head, 0)->item == 1 &&
          nth(front.head, 1) != NULL && nth(front.head, 1)->item == 2 &&
          nth(front.head, 2) != NULL && nth(front.head, 2)->item == 3,
        "Even length: front is 1 2 3");
    check(nth(back.head, 0) != NULL && nth(back.head, 0)->item == 4 &&
          nth(back.head, 1) != NULL && nth(back.head, 1)->item == 5 &&
          nth(back.head, 2) != NULL && nth(back.head, 2)->item == 6,
        "Even length: back is 4 5 6");
    empty_list(&ll);
    empty_list(&front);
    empty_list(&back);


    // Case 2: Odd length list gives front the extra item
    int a2[] = {1, 2, 3, 4, 5};
    build_list(&ll, a2, 5);
    front.head = NULL;
    front.size = 0;
    back.head = NULL;
    back.size = 0;
    frontBackSplitLinkedList(&ll, &front, &back);
    check(front.size == 3, "Odd length: front size is 3");
    check(back.size == 2, "Odd length: back size is 2");
    check(nth(front.head, 0) != NULL && nth(front.head, 0)->item == 1 &&
          nth(front.head, 1) != NULL && nth(front.head, 1)->item == 2 &&
          nth(front.head, 2) != NULL && nth(front.head, 2)->item == 3,
        "Odd length: front is 1 2 3");
    check(nth(back.head, 0) != NULL && nth(back.head, 0)->item == 4 &&
          nth(back.head, 1) != NULL && nth(back.head, 1)->item == 5,
        "Odd length: back is 4 5");
    empty_list(&ll);
    empty_list(&front);
    empty_list(&back);


    // Case 3: Single node list
    int a3[] = {7};
    build_list(&ll, a3, 1);
    front.head = NULL;
    front.size = 0;
    back.head = NULL;
    back.size = 0;
    frontBackSplitLinkedList(&ll, &front, &back);
    check(front.size == 1, "Single node: front size is 1");
    check(back.size == 0, "Single node: back size is 0");
    check(front.head != NULL && front.head->item == 7, "Single node: front is 7");
    check(back.head == NULL, "Single node: back is empty");
    empty_list(&ll);
    empty_list(&front);
    empty_list(&back);


    // Case 4: Empty list (isolated: frontBackSplitLinkedList could crash
    // internally on an empty list, so this runs in a child process)
    int empty_out[4];
    int empty_crashed = run_isolated(call_empty_list, empty_out, 4);
    check(!empty_crashed, "Empty list: does not crash");
    if (!empty_crashed) {
        check(empty_out[0] == 0, "Empty list: front size is 0");
        check(empty_out[1] == 0, "Empty list: back size is 0");
        check(empty_out[2] == 0, "Empty list: front is empty");
        check(empty_out[3] == 0, "Empty list: back is empty");
    }

    printf("\n%d / %d tests passed\n", tests_passed, tests_run);
    return 0;
}