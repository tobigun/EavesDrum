#include "note_event_queue.h"

#include <unity.h>

void fillQueue(NoteEventQueue& queue);

NoteEventQueue queue;

void setUp(void) {
  queue = NoteEventQueue();
}

void tearDown(void) {
  // clean stuff up here
}


void test_add_resets_isEmpty() {
  // GIVEN
  TEST_ASSERT_TRUE(queue.isEmpty());

  // WHEN
  queue.addNote(60, 100);

  // THEN
  TEST_ASSERT_FALSE(queue.isEmpty());
  TEST_ASSERT_EQUAL(1, queue.getSize());
}

void test_remove_sets_isEmpty() {
  // GIVEN
  queue.addNote(60, 100);

  // WHEN
  queue.removeOldestNote();

  // THEN
  TEST_ASSERT_TRUE(queue.isEmpty());
  TEST_ASSERT_EQUAL(0, queue.getSize());
}

void test_addNote() {
  // WHEN
  queue.addNote(60, 100);

  // THEN
  TEST_ASSERT_EQUAL_UINT8(60, queue.peekOldestNote().note);
  TEST_ASSERT_EQUAL_UINT8(100, queue.peekOldestNote().noteOnTimeMs);
}

void test_add_sets_isFull() {
  // WHEN
  fillQueue(queue);  

  // THEN
  TEST_ASSERT_TRUE(queue.isFull());
  TEST_ASSERT_EQUAL(NoteEventQueue::MAX_PENDING_NOTES, queue.getSize());
}

void test_remove_resets_isFull() {
  // GIVEN
  fillQueue(queue);

  // WHEN
  queue.removeOldestNote();

  // THEN
  TEST_ASSERT_FALSE(queue.isFull());
  TEST_ASSERT_EQUAL(NoteEventQueue::MAX_PENDING_NOTES - 1, queue.getSize());
}

void test_removeOldestNote() {
  // GIVEN
  queue.addNote(0, 1);
  queue.addNote(2, 3);

  // WHEN
  queue.removeOldestNote();  
  
  // THEN
  TEST_ASSERT_EQUAL_UINT8(2, queue.peekOldestNote().note);
  TEST_ASSERT_EQUAL_UINT8(3, queue.peekOldestNote().noteOnTimeMs);
  TEST_ASSERT_EQUAL(1, queue.getSize());
  TEST_ASSERT_FALSE(queue.isEmpty());
  TEST_ASSERT_FALSE(queue.isFull());
}

void test_remove_last_element_of_full_queue_does_not_crash() {
  // GIVEN
  fillQueue(queue);

  // WHEN
  queue.removeIndex(queue.getSize() - 1);

  // THEN
  TEST_ASSERT_FALSE(queue.isFull());
  TEST_ASSERT_EQUAL(NoteEventQueue::MAX_PENDING_NOTES - 1, queue.getSize());
}

void test_removeNote_removes_existing_note() {
  // GIVEN
  queue.addNote(0, 10);
  queue.addNote(1, 20);
  queue.addNote(2, 30);

  // WHEN
  bool removed = queue.removeNote(1);

  // THEN
  TEST_ASSERT_TRUE(removed);
  TEST_ASSERT_EQUAL(2, queue.getSize());
  TEST_ASSERT_EQUAL_UINT8(0, queue.peekOldestNote().note);

  queue.removeOldestNote();
  TEST_ASSERT_EQUAL_UINT8(2, queue.peekOldestNote().note);
}

void test_removeNote_returns_false_if_note_does_not_exist() {
  // GIVEN
  queue.addNote(0, 10);
  queue.addNote(1, 20);
  queue.addNote(2, 30);

  // WHEN
  bool removed = queue.removeNote(3);

  // THEN
  TEST_ASSERT_FALSE(removed);
  TEST_ASSERT_EQUAL(3, queue.getSize());
}


void fillQueue(NoteEventQueue& queue) {
  for (size_t i = 0; i < NoteEventQueue::MAX_PENDING_NOTES; i++) {
    TEST_ASSERT_FALSE(queue.isFull());
    queue.addNote(9, 10);
  }
}


int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_add_resets_isEmpty);
  RUN_TEST(test_remove_sets_isEmpty);
  RUN_TEST(test_add_sets_isFull);
  RUN_TEST(test_addNote);
  RUN_TEST(test_remove_resets_isFull);
  RUN_TEST(test_removeOldestNote);
  RUN_TEST(test_remove_last_element_of_full_queue_does_not_crash);
  RUN_TEST(test_removeNote_removes_existing_note);
  RUN_TEST(test_removeNote_returns_false_if_note_does_not_exist);
  return UNITY_END();
}
