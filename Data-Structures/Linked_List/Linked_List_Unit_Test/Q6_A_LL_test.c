#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define main original_main
#include "../Q6_A_LL.c"
#undef main


/* 
ASSUMPTIONS about moveMaxToFront
    1. Return value is 0 if list is empty
    2. If there are duplicate max values, the one with the first relative position will be moved
*/



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
    int result = moveMaxToFront(&head);
    int out[1] = {result};
    write(fd, out, sizeof(out));
}

void call_single_node_list(int fd) {
    ListNode *head = malloc(sizeof(ListNode));
    head->item = 42;
    head->next = NULL;
    int result = moveMaxToFront(&head);
    int out[2] = {result, head->item};
    write(fd, out, sizeof(out));
}


int main(void) {
    ListNode *head;
    int result;

    // Case 1: Max is in the middle
    int a1[] = {5, 3, 9, 2, 7};
    head = build_list(a1, 5);
    result = moveMaxToFront(&head);
    check(result == 1, "Max in middle: returns 1");
    check(nth(head, 0) != NULL && nth(head, 0)->item == 9 &&
          nth(head, 1) != NULL && nth(head, 1)->item == 5 &&
          nth(head, 2) != NULL && nth(head, 2)->item == 3 &&
          nth(head, 3) != NULL && nth(head, 3)->item == 2 &&
          nth(head, 4) != NULL && nth(head, 4)->item == 7,
        "Max in middle: becomes 9 5 3 2 7");
    free_list(head);


    // Case 2: Head is already the max
    int a2[] = {9, 3, 5, 2, 7};
    head = build_list(a2, 5);
    result = moveMaxToFront(&head);
    check(result == 1, "Head already max: returns 1");
    check(nth(head, 0) != NULL && nth(head, 0)->item == 9 &&
          nth(head, 1) != NULL && nth(head, 1)->item == 3 &&
          nth(head, 2) != NULL && nth(head, 2)->item == 5 &&
          nth(head, 3) != NULL && nth(head, 3)->item == 2 &&
          nth(head, 4) != NULL && nth(head, 4)->item == 7,
        "Head already max: list stays 9 3 5 2 7");
    free_list(head);


    // Case 3: Two nodes, second is bigger
    int a3[] = {3, 8};
    head = build_list(a3, 2);
    result = moveMaxToFront(&head);
    check(result == 1, "Two nodes, second bigger: returns 1");
    check(nth(head, 0) != NULL && nth(head, 0)->item == 8 &&
          nth(head, 1) != NULL && nth(head, 1)->item == 3,
        "Two nodes, second bigger: becomes 8 3");
    free_list(head);


    // Case 4: Duplicate max buried in the middle - the first occurrence
    // (index 1) should move to the front, ahead of the later duplicate.
    int a4[] = {3, 8, 5, 8, 2};
    head = build_list(a4, 5);
    result = moveMaxToFront(&head);
    check(result == 1, "Duplicate max buried: returns 1");
    check(nth(head, 0) != NULL && nth(head, 0)->item == 8 &&
          nth(head, 1) != NULL && nth(head, 1)->item == 3 &&
          nth(head, 2) != NULL && nth(head, 2)->item == 5 &&
          nth(head, 3) != NULL && nth(head, 3)->item == 8 &&
          nth(head, 4) != NULL && nth(head, 4)->item == 2,
        "Duplicate max buried: becomes 8 3 5 8 2");
    free_list(head);


    // Case 5: Duplicate max, first occurrence already at head - confirms
    // no unnecessary reordering happens when the first max is already front.
    int a5[] = {8, 3, 8, 2};
    head = build_list(a5, 4);
    result = moveMaxToFront(&head);
    check(result == 1, "Duplicate max at head: returns 1");
    check(nth(head, 0) != NULL && nth(head, 0)->item == 8 &&
          nth(head, 1) != NULL && nth(head, 1)->item == 3 &&
          nth(head, 2) != NULL && nth(head, 2)->item == 8 &&
          nth(head, 3) != NULL && nth(head, 3)->item == 2,
        "Duplicate max at head: list stays 8 3 8 2");
    free_list(head);


    // Case 6: Single-node list. A single node has nothing to compare against,
    // so a correct implementation should leave it alone: return 1 (max exists)
    // and keep it as the (only) node in the list.
    int single_out[2];
    int single_crashed = run_isolated(call_single_node_list, single_out, 2);
    check(!single_crashed, "Single node list: does not crash");
    if (!single_crashed) {
        check(single_out[0] == 1, "Single node list: returns 1");
        check(single_out[1] == 42, "Single node list: head item is still 42");
    }


    // Case 7: Empty list. No max exists, so this checks the function
    // reports that correctly (returns 0) without crashing.
    int empty_out[1];
    int empty_crashed = run_isolated(call_empty_list, empty_out, 1);
    check(!empty_crashed, "Empty list: does not crash");
    if (!empty_crashed) {
        check(empty_out[0] == 0, "Empty list: returns 0");
    }

    printf("\n%d / %d tests passed\n", tests_passed, tests_run);
    return 0;
}