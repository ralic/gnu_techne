#ifndef _STRUCTURES_H_
#define _STRUCTURES_H_

/* Doubly-linked lists. */

#define t_link_after(node, sibling)             \
    {                                           \
	(node)->left = (sibling);		\
	(node)->right = (sibling)->right;	\
                                                \
	if ((sibling)->right) {			\
	    (sibling)->right->left = (node);	\
	}					\
                                                \
	(sibling)->right = (node);		\
    }

#define t_link_before(node, sibling, head)	\
    {                                           \
	(node)->right = (sibling);		\
	(node)->left = (sibling)->left;		\
                                                \
	if ((sibling)->left) {			\
	    (sibling)->left->right = (node);	\
	} else {				\
	    *head = node;			\
	}					\
						\
	(sibling)->right = (node);		\
    }

#define t_link_between(node, brother, sister, head)	\
    {							\
        if (brother) {					\
            t_link_after(node, brother);		\
        } else {					\
            t_link_before(node, sister, head);		\
        }						\
    }

#define t_link_at_head(node, list)		\
    {                                           \
        (node)->left = NULL;			\
        (node)->right = *(list);		\
	(*(list))->left = (node);		\
        *(list) = (node);			\
    }

#define t_unlink_from(node, list)                       \
    {                                                   \
        if ((node)->left) {                             \
            (node)->left->right = (node)->right;        \
        } else {                                        \
            *list = (node)->right;                      \
        }                                               \
                                                        \
        if ((node)->right) {                            \
            (node)->right->left = (node)->left;         \
        }                                               \
                                                        \
        (node)->left = NULL;                            \
        (node)->right = NULL;                           \
    }    

/* Singly-linked lists. */

#define t_single_unlink_from(node, list)                                \
    {                                                                   \
	if (*(list) == (node)) {                                        \
            *(list) = (node)->next;                                     \
        } else {                                                        \
            typeof(node) p;                                             \
                                                                        \
            for (p = *(list) ; p->next != (node) ; p = p->next);        \
            p->next = (node)->next;                                     \
        }                                                               \
    }

#define t_single_link_at_head(node, list)       \
    {                                           \
        (node)->next = *list;                   \
        *list = (node);                         \
    }

/* Circular doubly-linked lists. */

#define t_circular_link_after(node, sibling)		\
    {							\
	(node)->left = (sibling);			\
	(node)->right = (sibling)->right;		\
	(sibling)->right->left = (node);		\
	(sibling)->right = (node);			\
    }

#define t_circular_link_before(node, sibling)			\
    {								\
	(node)->right = (sibling);				\
	(node)->left = (sibling)->left;				\
	(sibling)->left->right = (node);			\
	(sibling)->left = (node);				\
    }

#define t_circular_unlink(node)			\
    {                                           \
        (node)->left->right = (node)->right;    \
        (node)->right->left = (node)->left;     \
                                                \
        (node)->left = NULL;                    \
        (node)->right = NULL;                   \
    }    

#define t_make_circular(node) ((node)->left = (node),   \
                               (node)->right = (node),  \
                               (node))

#endif
