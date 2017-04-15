struct itree_node {
	unsigned long value;
	struct itree_node *left, *right;
};


extern struct itree_node *itree_insert(
	struct itree_node *root,
	unsigned long value
);

extern struct itree_node *itree_find(
	struct itree_node *root,
	unsigned long value
);
