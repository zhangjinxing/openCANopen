#include "tst.h"
#include "fff.h"
#include "canopen/sdo_req.h"
#include "sock.h"

DEFINE_FFF_GLOBALS;

FAKE_VOID_FUNC(mloop_iterate, struct mloop*);
FAKE_VALUE_FUNC(struct mloop*, mloop_default);
FAKE_VALUE_FUNC(struct mloop_idle*, mloop_idle_new, struct mloop*);
FAKE_VALUE_FUNC(int, mloop_idle_start, struct mloop_idle*);
FAKE_VALUE_FUNC(int, mloop_idle_unref, struct mloop_idle*);
FAKE_VOID_FUNC(mloop_idle_set_context, struct mloop_idle*, void*,
	       mloop_free_fn);
FAKE_VALUE_FUNC(void*, mloop_idle_get_context, const struct mloop_idle*);
FAKE_VOID_FUNC(mloop_idle_set_idle_fn, struct mloop_idle*, mloop_idle_fn);
FAKE_VOID_FUNC(mloop_idle_set_cond_fn, struct mloop_idle*, mloop_idle_cond_fn);
FAKE_VOID_FUNC(mloop_idle_set_priority, struct mloop_idle*, unsigned long);
FAKE_VALUE_FUNC(int, sdo_async_init, struct sdo_async*, const struct sock*,
		int);
FAKE_VALUE_FUNC(int, sdo_async_stop, struct sdo_async*);
FAKE_VOID_FUNC(sdo_async_destroy, struct sdo_async*);
FAKE_VALUE_FUNC(int, sdo_async_start, struct sdo_async*,
		const struct sdo_async_info*);

static int test_req_new_free()
{
	int dl_data = 1337;

	struct sdo_req_info info = {
		.type = SDO_REQ_DOWNLOAD,
		.index = 0x1234,
		.subindex = 42,
		.on_done = (sdo_req_fn)0xdeadbeef,
		.dl_data = &dl_data,
		.dl_size = sizeof(int)
	};

	struct sdo_req* req = sdo_req_new(&info);

	ASSERT_INT_EQ(SDO_REQ_DOWNLOAD, req->type);
	ASSERT_INT_EQ(0x1234, req->index);
	ASSERT_INT_EQ(42, req->subindex);
	ASSERT_PTR_EQ((void*)0xdeadbeef, req->on_done);

	ASSERT_INT_EQ(1337, *(int*)req->data.data);
	ASSERT_INT_EQ(sizeof(int), req->data.index);

	sdo_req_free(req);
	return 0;
}

static int test_req_queue_init_destroy()
{
	RESET_FAKE(sdo_async_init);
	sdo_async_init_fake.return_val = 0;

	struct mloop_idle* idle = (void*)0xdeadbeef;

	RESET_FAKE(mloop_idle_new);
	mloop_idle_new_fake.return_val = idle;

	struct sock sock = { .fd = 4, .type = SOCK_TYPE_CAN };

	struct sdo_req_queue queue;
	ASSERT_INT_EQ(0, sdo_req__queue_init(&queue, &sock, 42, 10, 0));

	ASSERT_INT_EQ(4, sdo_async_init_fake.arg1_val->fd);
	ASSERT_INT_EQ(SOCK_TYPE_CAN, sdo_async_init_fake.arg1_val->type);
	ASSERT_INT_EQ(42, sdo_async_init_fake.arg2_val);
	ASSERT_INT_EQ(10, queue.limit);

	sdo_req__queue_destroy(&queue);

	return 0;
}

static int test_req_queue_enqueue_dequeue()
{
	RESET_FAKE(sdo_async_init);
	sdo_async_init_fake.return_val = 0;

	struct sdo_req_queue queue;
	sdo_req__queue_init(&queue, 0, 0, 3, 0);

	struct sdo_req req[4];
	memset(req, 0, sizeof(req));

	ASSERT_INT_EQ(0, sdo_req_queue__enqueue(&queue, &req[0]));
	ASSERT_INT_EQ(0, sdo_req_queue__enqueue(&queue, &req[1]));
	ASSERT_INT_EQ(0, sdo_req_queue__enqueue(&queue, &req[2]));
	ASSERT_INT_LT(0, sdo_req_queue__enqueue(&queue, &req[3]));

	ASSERT_PTR_EQ(&queue, req[0].parent);
	ASSERT_PTR_EQ(&queue, req[1].parent);
	ASSERT_PTR_EQ(&queue, req[2].parent);
	ASSERT_PTR_EQ(NULL, req[3].parent);

	ASSERT_PTR_EQ(&req[0], sdo_req_queue__dequeue(&queue));
	ASSERT_PTR_EQ(&req[1], sdo_req_queue__dequeue(&queue));
	ASSERT_PTR_EQ(&req[2], sdo_req_queue__dequeue(&queue));
	ASSERT_PTR_EQ(NULL, sdo_req_queue__dequeue(&queue));

	sdo_req__queue_destroy(&queue);
	return 0;
}

static int test_req_queue_from_async()
{
	struct sdo_req_queue queue;
	ASSERT_PTR_EQ(&queue, sdo_req_queue__from_async(&queue.sdo_client));
	return 0;
}

int main()
{
	int r = 0;
	RUN_TEST(test_req_new_free);
	RUN_TEST(test_req_queue_init_destroy);
	RUN_TEST(test_req_queue_enqueue_dequeue);
	RUN_TEST(test_req_queue_from_async);
	return r;
}
