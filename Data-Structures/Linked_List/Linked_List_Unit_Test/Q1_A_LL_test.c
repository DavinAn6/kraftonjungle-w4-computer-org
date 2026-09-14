#include <stdio.h>
#include <stdlib.h>

#define main original_main
#include "../Q1_A_LL.c"
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


int main(void) {
    LinkedList ll;
    int result;

    // Case 1: Empty list
    ll.head = NULL;
    ll.size = 0;
    result = insertSortedLL(&ll, 5);
    check(result == 0, "Empty list: returns index 0");
    check(ll.size == 1, "Empty list: size becomes 1");
    check(ll.head != NULL && ll.head->item == 5, "Empty list: 5 becomes head");
    empty_list(&ll);


    // Case 2: Front insert
    insertSortedLL(&ll, 5);
    insertSortedLL(&ll, 8);
    result = insertSortedLL(&ll, 2);
    check(result == 0, "Front insert: returns index 0");
    check(nth(ll.head, 0) != NULL && nth(ll.head, 0)->item == 2,
        "Front insert: 2 becomes head");
    check(nth(ll.head, 1) != NULL && nth(ll.head, 1)->item == 5,
        "Front insert: 5 follows 2");
    check(nth(ll.head, 2) != NULL && nth(ll.head, 2)->item == 8,
        "Front insert: 8 remains at the end");
    empty_list(&ll);


    // Case 3: Middle insert
    insertSortedLL(&ll, 2);
    insertSortedLL(&ll, 5);
    insertSortedLL(&ll, 9);
    result = insertSortedLL(&ll, 7);
    check(result == 2, "Middle insert: returns index 2");
    check(nth(ll.head, 0) != NULL && nth(ll.head, 0)->item == 2,
        "Middle insert: 2 remains first");
    check(nth(ll.head, 1) != NULL && nth(ll.head, 1)->item == 5,
        "Middle insert: 5 remains second");
    check(nth(ll.head, 2) != NULL && nth(ll.head, 2)->item == 7,
        "Middle insert: 7 is inserted in the middle");
    check(nth(ll.head, 3) != NULL && nth(ll.head, 3)->item == 9,
        "Middle insert: 9 remains last");
    empty_list(&ll);


    // Case 4: End insert
    insertSortedLL(&ll, 2);
    insertSortedLL(&ll, 5);
    result = insertSortedLL(&ll, 9);
    check(result == 2, "End insert: returns index 2");
    check(nth(ll.head, 2) != NULL && nth(ll.head, 2)->item == 9,
        "End insert: 9 becomes the last node");
    empty_list(&ll);


    // Case 5: Duplicate insert
    insertSortedLL(&ll, 2);
    insertSortedLL(&ll, 5);
    insertSortedLL(&ll, 9);
    result = insertSortedLL(&ll, 5);
    check(result == -1, "Duplicate insert: returns -1");
    check(ll.size == 3, "Duplicate insert: size remains unchanged");
    check(nth(ll.head, 0) != NULL && nth(ll.head, 0)->item == 2 &&
          nth(ll.head, 1) != NULL && nth(ll.head, 1)->item == 5 &&
          nth(ll.head, 2) != NULL && nth(ll.head, 2)->item == 9,
        "Duplicate insert: list remains unchanged");
    empty_list(&ll);

    printf("\n%d / %d tests passed\n", tests_passed, tests_run);
    return 0;
}