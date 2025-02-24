#include <optional>
#include <atomic>
#include <thread>
#include <iostream>
#include <vector>
#include <chrono>

template <typename T>
class Queue {
public:
	struct Node {
		Node(T& val) : value(val), next(nullptr) {
		}
		T value;
		Node* next;
	};

	std::atomic<Node*> head;
	std::atomic<Node*> tail;

	void push(T val) {
		Node* new_node = new Node(val);
		Node* current_tail = tail.load(std::memory_order_acquire);
		if (!current_tail) {
			if (tail.compare_exchange_weak(current_tail, new_node, std::memory_order_release)) {
				head.store(new_node, std::memory_order_release);
				return;
			}
		}

		do {
			current_tail->next = new_node;
		} while (!tail.compare_exchange_weak(current_tail, new_node, std::memory_order_release));
	}

	std::optional<T> pop() {
		Node* current_head;
		do {
			current_head = head.load(std::memory_order_acquire);
			if (!current_head) {
				return std::nullopt;
			}

		} while (!head.compare_exchange_weak(current_head, current_head->next, std::memory_order_release));

		T ret_value = current_head->value;
		return ret_value;
	}

	void print() {
		Node* ptr = head.load(std::memory_order_relaxed);
		if (!ptr) std::cout << "queue is empty";
		while (ptr) {
			std::cout << ptr->value;
			ptr = ptr->next;
		}
	}
};

int main() {

	using namespace std::chrono_literals;
	Queue<int> lock_free_queue;
	std::vector<std::thread> threads;

	for (int idx = 0; idx < 100; ++idx) {
		threads.emplace_back(std::thread([&] {
			int i = 100;
			while (i) {
				lock_free_queue.push(i);
				lock_free_queue.pop();
				--i;
			}

			}));
	}

	for (int idx = 0; idx < 100; ++idx) {
		threads[idx].join();
	}

	lock_free_queue.print();
	return 0;
}
