#ifndef __DIGTREE_H__
#define __DIGTREE_H__
//
// Simon Banks Digit tree index and methods..
// //
// Digit tree index.. This is super fast/effcient for MSISDN lookups
// 
#include <stdlib.h>
#include <stdbool.h>

typedef struct DigTree
{
	void*				data;		// Store anything against this DigTree
	unsigned			depth:5;	// Depth into re
	unsigned			maxdigits:5;// 0 - maxdigits value range
	struct DigTree*		tree[0];	// Placeholder for allocated space
} *DigTree;

// Methods which can be used to build a DigTree.
// //
extern DigTree	initDigTree(size_t);
extern DigTree	assignDigTree(DigTree*, const char[], void*);
extern DigTree	findDigTree(DigTree, const char[], bool, int);
extern void		releaseDigTree(DigTree*);

#endif // __DIGTREE_H__
