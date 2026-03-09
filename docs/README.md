# MQ++

## Overview

MQ++ is a message queue broker based on publishers and consumers.
The software provides message durability, regex based topic subscriptions, and automatic retry delivery.

**Core properties:**
- Producers do not have to wait for consumers for acknowledgement
- Messages need to be read at least once
- Topic subscriptions via regexes
- Durability through message persistence

## Components

### Broker ([`src/client/broker.hpp`](../src/client/broker.hpp))

Responsible for communication between publishers and consumer and publisher.

**HTTP endpoints:**

| Endpoint       | Purpose                                 |
|----------------|-----------------------------------------|
| `POST /publish`     | Accept a message from a publisher       |
| `POST /subscribe`   | Register a consumer with topic patterns |
| `POST /acknowledge` | Mark a message as received by consumer  |

**Startup sequence**
1. Load all persisted `.mqpp` message files from `storage_dir`
2. Start dispatcher retry background thread for not delivered messages

### Publisher ([`src/client/publisher.hpp`](../src/client/publisher.hpp))

Sends an HTTP POST to `/publish` and returns the assigned message ID.

### Consumer ([`src/client/consumer.hpp`](../src/client/consumer.hpp))

Receives incoming messages from the broker.

**Lifecycle**
1. Starts listening on `listen_port`
2. Creates a POST subscription request to broker (`consumer_id`, `callback_url`, `patterns`)
3. Broker immediately dispatches any matching persisted messages
4. Receives messages at `POST /receive`, the user message handler gets invokes and clients sends acknowledgment

### Message ([`src/message/message.hpp`](../src/message/message.hpp))

Domain model for a single message.

| Field     | Type           | Notes               |
|-----------|----------------|---------------------|
| `id`      | `MessageId`    | UUID                |
| `topic`   | `string`       | Topic string regex  |
| `payload` | `string`       | Content             |
|`timestamp`| `time_point`   | Creation time       |

---

### MessageStore ([`src/persistence/message_store.hpp`](../src/persistence/message_store.hpp))

Persists messages to disk.

**File format** (one `.mqpp` file per message):
```
message=<serialized_message>
PENDING=consumer1,consumer2
ACKNOWLEDGED=consumer3
LAST_RETRY=1770000000
```

---

### SubscriptionManager ([`src/subscription/subscription_manager.hpp`](../src/subscription/subscription_manager.hpp))

Registry mapping consumer IDs to regex topic patterns.

- Subscriptions stored as `(consumer_id, std::regex)` pairs

---

### MessageDispatcher ([`src/dispatching/message_dispatcher.hpp`](../src/dispatching/message_dispatcher.hpp))

Handles the asynchronous delivery.

---

### HttpTransport ([`src/network/http_transport.hpp`](../src/network/http_transport.hpp))

Implements the `ITransport` interface using `cpp-httplib`

---

## Message Flows

### Publish

The publisher sends a `POST /publish` to the broker.
The broker matches the message topic against all active subscriptions,
persists the message to disk, and attempts delivery via `POST /receive` to every matched consumer.
The broker returns the assigned `message_id` to the publisher.
Each consumer sends a `POST /acknowledge` back to the broker,
which marks the message as acknowledged and deletes the file once all targeted consumers have acknowledged delivery.

### Subscribe

The consumer starts its listening and sends a `POST /subscribe` to the broker with its `consumer_id`, `callback_url`, and topic patterns.
The broker registers the patterns, scans persisted messages for any that match, adds the consumer to their pending sets,
and sends those messages via `POST /receive`.

### Retry

A background thread in the dispatcher periodically polls the `MessageStore` for messages with pending consumers
that have not been recently attempted. For each such message, it sends `POST /receive` to the consumer's callback URL
and updates the `last_retry` timestamp. This repeats on an interval; only messages older than `retry_interval` are dispatched.

---

## Text Format

All HTTP bodies use a custom `key=value|key=value` text format.

## Persistence & Durability

- Messages are written to disk before dispatch attempts begin
- On broker restart, all `.mqpp` files are loaded; delivery resumes automatically
- A message file is deleted only when targeted consumers have acknowledged
- Consumers that subscribe after a message is published are matched against persisted messages and added to the pending set retroactively