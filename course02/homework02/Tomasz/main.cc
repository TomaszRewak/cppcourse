#include "message_throttler_commons.hpp"

#include <functional>
#include <iostream>
#include <thread>

using Message = int;
using UserId = std::size_t;

void buffer_empty_test()
{
	short_circular_buffer<UserId, 4> buffer;

	assert(buffer.empty());
}

void buffer_full_test()
{
	short_circular_buffer<UserId, 4> buffer;

	buffer.push(1);
	buffer.push(2);
	buffer.push(3);
	buffer.push(4);

	assert(buffer.full());
	assert(buffer.front() == 1);
}

void buffer_pop_test()
{
	short_circular_buffer<UserId, 4> buffer;

	buffer.push(1);
	buffer.push(2);
	buffer.push(3);
	buffer.push(4);
	buffer.pop();
	buffer.pop();

	assert(!buffer.full());
	assert(buffer.front() == 3);
}

void buffer_cycle_test()
{
	short_circular_buffer<UserId, 4> buffer;

	buffer.push(1);
	buffer.push(2);
	buffer.push(3);
	buffer.push(4);
	buffer.pop();
	buffer.pop();
	buffer.push(5);
	buffer.push(6);
	buffer.pop();
	buffer.pop();
	buffer.pop();

	assert(!buffer.full());
	assert(buffer.front() == 6);
}

void throttler_test()
{
	Message last_consumed_message = 0;

	auto message_bus = make_message_throttler<UserId, 4, Message>(
		[&](const Message& message) { last_consumed_message = message; }
	);

	message_bus.from(1).send(1);
	message_bus.from(1).send(2);
	message_bus.from(1).send(3);

	assert(last_consumed_message == 3);
}

void throttler_dispose_test()
{
	Message last_consumed_message = 0;
	Message last_disposed_message = 0;

	auto message_bus = make_message_throttler<UserId, 4, Message>(
		[&](const Message& message) { last_consumed_message = message; },
		[&](const Message& message) { last_disposed_message = message; }
	);

	message_bus.from(1).send(1);
	message_bus.from(1).send(2);
	message_bus.from(1).send(3);
	message_bus.from(1).send(4);
	message_bus.from(1).send(5);
	message_bus.from(1).send(6);

	assert(last_consumed_message == 4);
	assert(last_disposed_message == 6);
}

void throttler_single_interface_test()
{
	Message last_consumed_message = 0;
	Message last_disposed_message = 0;

	auto message_bus = make_message_throttler<UserId, 4, Message>(
		[&](const Message& message) { last_consumed_message = message; },
		[&](const Message& message) { last_disposed_message = message; }
	);

	message_bus.from(1)
		.send(1)
		.send(2)
		.send(3)
		.send(4)
		.send(5)
		.send(6);

	assert(last_consumed_message == 4);
	assert(last_disposed_message == 6);
}

void throttler_multiple_users_test()
{
	Message last_consumed_message = 0;
	Message last_disposed_message = 0;

	auto message_bus = make_message_throttler<UserId, 4, Message>(
		[&](const Message& message) { last_consumed_message = message; },
		[&](const Message& message) { last_disposed_message = message; }
	);

	message_bus.from(1).send(1);
	message_bus.from(1).send(2);
	message_bus.from(1).send(3);
	message_bus.from(1).send(4);
	message_bus.from(1).send(5);
	message_bus.from(2).send(7);
	message_bus.from(2).send(8);

	assert(last_consumed_message == 8);
	assert(last_disposed_message == 5);
}

void throttler_timeout_test()
{
	Message last_consumed_message = 0;
	Message last_disposed_message = 0;

	auto message_bus = make_message_throttler<UserId, 4, Message>(
		[&](const Message& message) { last_consumed_message = message; },
		[&](const Message& message) { last_disposed_message = message; },
		[](std::chrono::microseconds now, std::chrono::microseconds timestamp) { return false; }
	);

	message_bus.from(1).send(1);
	message_bus.from(1).send(2);
	message_bus.from(1).send(3);
	message_bus.from(1).send(4);
	message_bus.from(1).send(5);
	message_bus.from(1).send(6);

	assert(last_consumed_message == 6);
	assert(last_disposed_message == 0);
}

void throttler_custom_timestamper_test()
{
	std::size_t timestamp = 0;

	Message last_consumed_message = 0;
	Message last_disposed_message = 0;

	auto message_bus = make_message_throttler<UserId, 4, Message, std::size_t>(
		[&](const Message& message) { last_consumed_message = message; },
		[&](const Message& message) { last_disposed_message = message; },
		[&](std::size_t now, std::size_t timestamp) { return timestamp > 0; },
		[&]() { return timestamp++; }
	);

	message_bus.from(1).send(1);
	message_bus.from(1).send(2);
	message_bus.from(1).send(3);
	message_bus.from(1).send(4);
	message_bus.from(1).send(5);
	message_bus.from(1).send(6);

	assert(last_consumed_message == 5);
	assert(last_disposed_message == 6);
}

void throttler_chrono_test()
{
	Message last_consumed_message = 0;
	Message last_disposed_message = 0;

	auto message_bus = make_message_throttler<UserId, 4, Message>(
		[&](const Message& message) { last_consumed_message = message; },
		[&](const Message& message) { last_disposed_message = message; },
		chrono_timestamp_threshold{ std::chrono::milliseconds{ 100 } }
	);

	message_bus.from(1).send(1);
	message_bus.from(1).send(2);
	message_bus.from(1).send(3);
	message_bus.from(1).send(4);
	message_bus.from(1).send(5);

	std::this_thread::sleep_for(std::chrono::milliseconds{ 50 });

	message_bus.from(1).send(6);
	message_bus.from(1).send(7);

	std::this_thread::sleep_for(std::chrono::milliseconds{ 50 });

	message_bus.from(1).send(8);
	message_bus.from(1).send(9);

	assert(last_consumed_message == 9);
	assert(last_disposed_message == 7);
}

void throttler_chrono_partial_test()
{
	Message last_consumed_message = 0;
	Message last_disposed_message = 0;

	auto message_bus = make_message_throttler<UserId, 4, Message>(
		[&](const Message& message) { last_consumed_message = message; },
		[&](const Message& message) { last_disposed_message = message; },
		chrono_timestamp_threshold{ std::chrono::milliseconds{ 100 } }
	);

	message_bus.from(1).send(1);
	message_bus.from(1).send(2);

	std::this_thread::sleep_for(std::chrono::milliseconds{ 50 });

	message_bus.from(1).send(3);
	message_bus.from(1).send(4);

	std::this_thread::sleep_for(std::chrono::milliseconds{ 50 });

	message_bus.from(1).send(5);
	message_bus.from(1).send(6);
	message_bus.from(1).send(7);

	assert(last_consumed_message == 6);
	assert(last_disposed_message == 7);
}

void throttler_chrono_full_cycle_test()
{
	Message last_consumed_message = 0;
	Message last_disposed_message = 0;

	auto message_bus = make_message_throttler<UserId, 4, Message>(
		[&](const Message& message) { last_consumed_message = message; },
		[&](const Message& message) { last_disposed_message = message; },
		chrono_timestamp_threshold{ std::chrono::milliseconds{ 100 } }
	);

	message_bus.from(1).send(1);
	message_bus.from(1).send(2);
	message_bus.from(1).send(3);
	message_bus.from(1).send(4);

	std::this_thread::sleep_for(std::chrono::milliseconds{ 100 });

	message_bus.from(1).send(5);
	message_bus.from(1).send(6);
	message_bus.from(1).send(7);
	message_bus.from(1).send(8);
	message_bus.from(1).send(9);

	assert(last_consumed_message == 8);
	assert(last_disposed_message == 9);
}

void throttler_chrono_muliple_users_test()
{
	Message last_consumed_message = 0;
	Message last_disposed_message = 0;

	auto message_bus = make_message_throttler<UserId, 4, Message>(
		[&](const Message& message) { last_consumed_message = message; },
		[&](const Message& message) { last_disposed_message = message; },
		chrono_timestamp_threshold{ std::chrono::milliseconds{ 100 } }
	);

	message_bus.from(1).send(1);
	message_bus.from(1).send(2);
	message_bus.from(2).send(3);
	message_bus.from(2).send(4);

	std::this_thread::sleep_for(std::chrono::milliseconds{ 50 });

	message_bus.from(1).send(5);
	message_bus.from(1).send(6);
	message_bus.from(1).send(7);
	message_bus.from(1).send(8);
	message_bus.from(2).send(9);

	assert(last_consumed_message == 9);
	assert(last_disposed_message == 8);
}

int main()
{
	buffer_empty_test();
	buffer_full_test();
	buffer_pop_test();
	buffer_cycle_test();
	throttler_test();
	throttler_dispose_test();
	throttler_single_interface_test();
	throttler_multiple_users_test();
	throttler_timeout_test();
	throttler_custom_timestamper_test();
	throttler_chrono_test();
	throttler_chrono_partial_test();
	throttler_chrono_full_cycle_test();
	throttler_chrono_muliple_users_test();
}