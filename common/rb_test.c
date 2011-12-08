#include <stdio.h>

#include "time.h"
#include "rbtree.h"

struct _obj_t {
	int key;
	struct rb_node node;
	char data[32];
};

typedef struct _obj_t obj_t;

obj_t*
my_search(struct rb_root *root, int key)
{
	struct rb_node *node = root->rb_node;

	while (node) {
		obj_t *data = container_of(node, struct _obj_t, node);
		int result;

		result = key - data->key;

		if (result < 0)
			node = node->rb_left;
		else if (result > 0)
			node = node->rb_right;
		else
			return data;
	}

	return NULL;
}

int
rb_insert_obj(struct rb_root *root, obj_t *data)
{
	obj_t *this;
	struct rb_node **p = &(root->rb_node), *parent = NULL;

	while (*p)
	{
		parent = *p;
		this = container_of(*p, struct _obj_t, node);
		int result = data->key - this->key;

		parent = *p;
		if (result < 0)
			p = &(*p)->rb_left;
		else if (result > 0)
			p = &(*p)->rb_right;
		else
			return -1;
	}

	rb_link_node(&data->node, parent, p);
	rb_insert_color(&data->node, root);

	return 0;
}
#define MAX_SET 100000
#define SEARCH_SET 5
_time_t duration[MAX_SET];
int search_set[SEARCH_SET] = {432, 51236, 234124, 432342323, 4232};
int main (void)
{
	struct rb_root mytree = RB_ROOT;
	obj_t *obj = NULL;
	int i, ret;
	_time_t t1, t2,t3;

	for (i = 0; i< MAX_SET; i++) {
		obj_t *o1 = malloc(sizeof(obj_t));
		o1->key = i;
		t1 = rdtsc();
		ret = rb_insert_obj(&mytree, o1);
		t2 = rdtsc();
		duration[i] = tick_diff_usecs(t1, t2);
		if (ret != 0)
			printf("failed to insert %d in tree\n", i);
	}

	for (i = 0; i < SEARCH_SET; i++) {
		t1 = rdtsc();
		obj = my_search(&mytree, search_set[i]);
		t2 = rdtsc();
		t3 = tick_diff_usecs(t1, t2);

		if (obj) {
			printf("found obj %d in %llu usecs\n", obj->key, t3);
		} else
			printf("obj not found in %llu usecs\n", t3);
	}
	printf("sleeping for 10 secs\n");
	sleep(10);

	for (i = 0; i< MAX_SET; i++) {
		if (duration[i] > 0)
		printf("obj %d took %llu to insert\n", i, duration[i]);
	}
	for (i = 0; i< MAX_SET; i++) {
		if (duration[i] > 5)
		printf("obj %d took %llu to insert\n", i, duration[i]);
	}

	printf("sleeping for 10 secs\n");
	sleep(10);
	for (i = 0; i< MAX_SET; i++) {
		t1 = rdtsc();
		obj = my_search(&mytree, i);
		if (obj) {
			rb_erase(&obj->node, &mytree);
			free(obj);
		}
		t2 = rdtsc();
		t3 = tick_diff_usecs(t1, t2);
		duration[i] = tick_diff_usecs(t1, t2);
	}
	for (i = 0; i< MAX_SET; i++) {
		if (duration[i] > 5)
		printf("obj %d took %llu to find/delete\n", i, duration[i]);
	}
	 
	return 0;
}
