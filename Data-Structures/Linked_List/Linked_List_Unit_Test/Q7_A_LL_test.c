#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define main original_main
#include "../Q7_A_LL.c"
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

// Runs fn(fd) in a child process. fn should write its result(s) to fd before
// exiting normally. Returns 1 if the child crashed (segfault, etc.) instead of
// taking the whole test binary down with it, so risky edge cases can still be
// checked and reported like any other test. On a crash, *out is left untouched.
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

ListNode *build_list(int *arr, int n) {
    ListNode *head = NULL;
    ListNode *tail = NULL;
    for (int i = 0; i < n; i++) {
        ListNode *node = malloc(sizeof(ListNode));
        node->item = arr[i];
        node->next = NULL;
        if (tail == NULL) {
            head = node;
        } else {
            tail->next = node;
        }
        tail = node;
    }
    return head;
}

void free_list(ListNode *head) {
    while (head != NULL) {
        ListNode *next = head->next;
        free(head);
        head = next;
    }
}

void call_empty_list(int fd) {
    ListNode *head = NULL;
    RecursiveReverse(&head);
    int out[1] = {head == NULL ? 1 : 0};
    write(fd, out, sizeof(out));
}

void call_single_node_list(int fd) {
    ListNode *head = malloc(sizeof(ListNode));
    head->item = 9;
    head->next = NULL;
    RecursiveReverse(&head);
    int out[2] = {head->item, head->next == NULL ? 1 : 0};
    write(fd, out, sizeof(out));
}


int main(void) {
    ListNode *head;

    // Case 1: Odd length list
    int a1[] = {1, 2, 3, 4, 5};
    head = build_list(a1, 5);
    RecursiveReverse(&head);
    check(nth(head, 0) != NULL && nth(head, 0)->item == 5 &&
          nth(head, 1) != NULL && nth(head, 1)->item == 4 &&
          nth(head, 2) != NULL && nth(head, 2)->item == 3 &&
          nth(head, 3) != NULL && nth(head, 3)->item == 2 &&
          nth(head, 4) != NULL && nth(head, 4)->item == 1,
        "Odd length: becomes 5 4 3 2 1");
    free_list(head);


    // Case 2: Even length list
    int a2[] = {1, 2, 3, 4};
    head = build_list(a2, 4);
    RecursiveReverse(&head);
    check(nth(head, 0) != NULL && nth(head, 0)->item == 4 &&
          nth(head, 1) != NULL && nth(head, 1)->item == 3 &&
          nth(head, 2) != NULL && nth(head, 2)->item == 2 &&
          nth(head, 3) != NULL && nth(head, 3)->item == 1,
        "Even length: becomes 4 3 2 1");
    free_list(head);


    // Case 3: Two-node list
    int a3[] = {7, 3};
    head = build_list(a3, 2);
    RecursiveReverse(&head);
    check(nth(head, 0) != NULL && nth(head, 0)->item == 3 &&
          nth(head, 1) != NULL && nth(head, 1)->item == 7,
        "Two nodes: becomes 3 7");
    free_list(head);


    // Case 4: Single-node list. Nothing to reverse, so a correct
    // implementation should leave the single node exactly as it is.
    int single_out[2];
    int single_crashed = run_isolated(call_single_node_list, single_out, 2);
    check(!single_crashed, "Single node list: does not crash");
    if (!single_crashed) {
        check(single_out[0] == 9, "Single node list: item stays 9");
        check(single_out[1] == 1, "Single node list: next is still NULL");
    }


    // Case 5: Empty list. There's no node to reverse, so this only checks
    // that a correct implementation handles it without crashing.
    int empty_out[1];
    int empty_crashed = run_isolated(call_empty_list, empty_out, 1);
    check(!empty_crashed, "Empty list: does not crash");

    printf("\n%d / %d tests passed\n", tests_passed, tests_run);
    return 0;
}