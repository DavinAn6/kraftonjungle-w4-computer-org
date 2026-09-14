#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define main original_main
#include "../Q4_A_LL.c"
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

// out layout: [size, head != NULL ? 1 : 0]
void call_empty_list(int fd) {
    LinkedList ll;
    ll.head = NULL;
    ll.size = 0;
    moveEvenItemsToBack(&ll);
    int out[2] = {ll.size, ll.head != NULL ? 1 : 0};
    write(fd, out, sizeof(out));
}


int main(void) {
    LinkedList ll;

    // Case 1: Mixed items
    int a1[] = {2, 3, 4, 7, 15, 18};
    build_list(&ll, a1, 6);
    moveEvenItemsToBack(&ll);
    check(ll.size == 6, "Mixed: size unchanged");
    check(nth(ll.head, 0) != NULL && nth(ll.head, 0)->item == 3 &&
          nth(ll.head, 1) != NULL && nth(ll.head, 1)->item == 7 &&
          nth(ll.head, 2) != NULL && nth(ll.head, 2)->item == 15 &&
          nth(ll.head, 3) != NULL && nth(ll.head, 3)->item == 2 &&
          nth(ll.head, 4) != NULL && nth(ll.head, 4)->item == 4 &&
          nth(ll.head, 5) != NULL && nth(ll.head, 5)->item == 18,
        "Mixed: becomes 3 7 15 2 4 18");
    empty_list(&ll);


    // Case 2: All odd items
    int a2[] = {1, 3, 5, 7};
    build_list(&ll, a2, 4);
    moveEvenItemsToBack(&ll);
    check(ll.size == 4, "All odd: size unchanged");
    check(nth(ll.head, 0) != NULL && nth(ll.head, 0)->item == 1 &&
          nth(ll.head, 1) != NULL && nth(ll.head, 1)->item == 3 &&
          nth(ll.head, 2) != NULL && nth(ll.head, 2)->item == 5 &&
          nth(ll.head, 3) != NULL && nth(ll.head, 3)->item == 7,
        "All odd: list stays 1 3 5 7");
    empty_list(&ll);


    // Case 3: All even items
    int a3[] = {2, 4, 6, 8};
    build_list(&ll, a3, 4);
    moveEvenItemsToBack(&ll);
    check(ll.size == 4, "All even: size unchanged");
    check(nth(ll.head, 0) != NULL && nth(ll.head, 0)->item == 2 &&
          nth(ll.head, 1) != NULL && nth(ll.head, 1)->item == 4 &&
          nth(ll.head, 2) != NULL && nth(ll.head, 2)->item == 6 &&
          nth(ll.head, 3) != NULL && nth(ll.head, 3)->item == 8,
        "All even: list stays 2 4 6 8");
    empty_list(&ll);


    // Case 4: Head starts even, evens scattered
    int a4[] = {4, 1, 3, 2, 9};
    build_list(&ll, a4, 5);
    moveEvenItemsToBack(&ll);
    check(ll.size == 5, "Even head: size unchanged");
    check(nth(ll.head, 0) != NULL && nth(ll.head, 0)->item == 1 &&
          nth(ll.head, 1) != NULL && nth(ll.head, 1)->item == 3 &&
          nth(ll.head, 2) != NULL && nth(ll.head, 2)->item == 9 &&
          nth(ll.head, 3) != NULL && nth(ll.head, 3)->item == 4 &&
          nth(ll.head, 4) != NULL && nth(ll.head, 4)->item == 2,
        "Even head: becomes 1 3 9 4 2");
    empty_list(&ll);


    // Case 5: Empty list (isolated: moveEvenItemsToBack could crash
    // internally on an empty list, so this runs in a child process)
    int empty_out[2];
    int empty_crashed = run_isolated(call_empty_list, empty_out, 2);
    check(!empty_crashed, "Empty list: does not crash");
    if (!empty_crashed) {
        check(empty_out[1] == 0, "Empty list: head stays NULL");
        check(empty_out[0] == 0, "Empty list: size stays 0");
    }

    printf("\n%d / %d tests passed\n", tests_passed, tests_run);
    return 0;
}