#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define main original_main
#include "../Q2_A_LL.c"
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

// out layout: [ll1.size, ll2.size, ll2.head item or -1 if NULL]
void call_empty_ll1(int fd) {
    LinkedList ll1, ll2;
    int a[] = {4, 5, 6};
    ll1.head = NULL;
    ll1.size = 0;
    build_list(&ll2, a, 3);
    alternateMergeLinkedList(&ll1, &ll2);
    int out[3] = {ll1.size, ll2.size, ll2.head != NULL ? ll2.head->item : -1};
    write(fd, out, sizeof(out));
}

// out layout: [ll1.size, ll2.size, ll1 items 1..3, or -1 for missing nodes]
void call_empty_ll2(int fd) {
    LinkedList ll1, ll2;
    int a[] = {1, 2, 3};
    build_list(&ll1, a, 3);
    ll2.head = NULL;
    ll2.size = 0;
    alternateMergeLinkedList(&ll1, &ll2);
    ListNode *n0 = nth(ll1.head, 0);
    ListNode *n1 = nth(ll1.head, 1);
    ListNode *n2 = nth(ll1.head, 2);
    int out[5] = {
        ll1.size, ll2.size,
        n0 != NULL ? n0->item : -1,
        n1 != NULL ? n1->item : -1,
        n2 != NULL ? n2->item : -1
    };
    write(fd, out, sizeof(out));
}


int main(void) {
    LinkedList ll1, ll2;

    // Case 1: LL2 longer than available alternate slots
    int a1[] = {1, 2, 3};
    int a2[] = {4, 5, 6, 7};
    build_list(&ll1, a1, 3);
    build_list(&ll2, a2, 4);
    alternateMergeLinkedList(&ll1, &ll2);
    check(ll1.size == 6, "Longer LL2: ll1 size becomes 6");
    check(ll2.size == 1, "Longer LL2: ll2 size becomes 1");
    check(nth(ll1.head, 0) != NULL && nth(ll1.head, 0)->item == 1 &&
          nth(ll1.head, 1) != NULL && nth(ll1.head, 1)->item == 4 &&
          nth(ll1.head, 2) != NULL && nth(ll1.head, 2)->item == 2 &&
          nth(ll1.head, 3) != NULL && nth(ll1.head, 3)->item == 5 &&
          nth(ll1.head, 4) != NULL && nth(ll1.head, 4)->item == 3 &&
          nth(ll1.head, 5) != NULL && nth(ll1.head, 5)->item == 6,
        "Longer LL2: ll1 becomes 1 4 2 5 3 6");
    check(ll2.head != NULL && ll2.head->item == 7, "Longer LL2: ll2 retains leftover 7");
    empty_list(&ll1);
    empty_list(&ll2);


    // Case 2: LL2 exactly fills every alternate slot
    int a3[] = {1, 2, 3};
    int a4[] = {4, 5, 6};
    build_list(&ll1, a3, 3);
    build_list(&ll2, a4, 3);
    alternateMergeLinkedList(&ll1, &ll2);
    check(ll1.size == 6, "Exact fit: ll1 size becomes 6");
    check(ll2.size == 0, "Exact fit: ll2 size becomes 0");
    check(nth(ll1.head, 0) != NULL && nth(ll1.head, 0)->item == 1 &&
          nth(ll1.head, 1) != NULL && nth(ll1.head, 1)->item == 4 &&
          nth(ll1.head, 2) != NULL && nth(ll1.head, 2)->item == 2 &&
          nth(ll1.head, 3) != NULL && nth(ll1.head, 3)->item == 5 &&
          nth(ll1.head, 4) != NULL && nth(ll1.head, 4)->item == 3 &&
          nth(ll1.head, 5) != NULL && nth(ll1.head, 5)->item == 6,
        "Exact fit: ll1 becomes 1 4 2 5 3 6");
    check(ll2.head == NULL, "Exact fit: ll2 becomes empty");
    empty_list(&ll1);
    empty_list(&ll2);


    // Case 3: LL1 shorter than LL2, only 1 node
    int a5[] = {1};
    int a6[] = {4, 5};
    build_list(&ll1, a5, 1);
    build_list(&ll2, a6, 2);
    alternateMergeLinkedList(&ll1, &ll2);
    check(ll1.size == 2, "Single node LL1: ll1 size becomes 2");
    check(ll2.size == 1, "Single node LL1: ll2 size becomes 1");
    check(nth(ll1.head, 0) != NULL && nth(ll1.head, 0)->item == 1 &&
          nth(ll1.head, 1) != NULL && nth(ll1.head, 1)->item == 4,
        "Single node LL1: ll1 becomes 1 4");
    check(ll2.head != NULL && ll2.head->item == 5, "Single node LL1: ll2 retains leftover 5");
    empty_list(&ll1);
    empty_list(&ll2);


    // Case 4: LL1 is empty (isolated: alternateMergeLinkedList could crash
    // internally on an empty ll1, so this runs in a child process)
    int empty_ll1_out[3];
    int empty_ll1_crashed = run_isolated(call_empty_ll1, empty_ll1_out, 3);
    check(!empty_ll1_crashed, "Empty LL1: does not crash");
    if (!empty_ll1_crashed) {
        check(empty_ll1_out[0] == 0, "Empty LL1: ll1 size stays 0");
        check(empty_ll1_out[1] == 3, "Empty LL1: ll2 size stays 3");
        check(empty_ll1_out[2] == 4, "Empty LL1: ll2 remains unchanged");
    }


    // Case 5: LL2 is empty (isolated: alternateMergeLinkedList could crash
    // internally on an empty ll2, so this runs in a child process)
    int empty_ll2_out[5];
    int empty_ll2_crashed = run_isolated(call_empty_ll2, empty_ll2_out, 5);
    check(!empty_ll2_crashed, "Empty LL2: does not crash");
    if (!empty_ll2_crashed) {
        check(empty_ll2_out[0] == 3, "Empty LL2: ll1 size stays 3");
        check(empty_ll2_out[1] == 0, "Empty LL2: ll2 size stays 0");
        check(empty_ll2_out[2] == 1 &&
              empty_ll2_out[3] == 2 &&
              empty_ll2_out[4] == 3,
            "Empty LL2: ll1 remains unchanged");
    }

    printf("\n%d / %d tests passed\n", tests_passed, tests_run);
    return 0;
}