#include "fc_tools.h"
#include "fc_xhash.h"


/* list of prime numbers to compute size of the hash-table */
static const unsigned int fc_primes[] ={
    109,
    163,
    251,
    367,
    557,
    823,
    1237,
    1861,
    2777,
    4177,
    6247,
    9371,
    14057,
    21089,
    31627,
    47431,
    71143,
    106721,
    160073,
    240101,
    360163,
    540217,
    810343,
    1215497,
    1823231,
    2734867,
    4102283,
    6153409,
    9230113,
    13845163,
};
static const unsigned fc_nprimes = sizeof (fc_primes) / sizeof (fc_primes[0]);

/* gives the nearest prime number */
unsigned int fc_spaced_primes_closest(unsigned int num)
{
    unsigned int i;

    for (i = 0; i < fc_nprimes; i++)
        if (fc_primes[i] > num)
            return (fc_primes[i]);

    return (fc_primes[fc_nprimes - 1]);
}
/* gives the first prime number */
#define fc_first_prime_number() (fc_primes[0])

/* (!) create a hash-table */
FC_Hash *fc_hash_new(void)
{
    FC_Hash *hash;
    int i;

    hash = malloc(sizeof (FC_Hash));
    if (hash == NULL)
    {
        fc_message("cannot allocate a hash-table");
        return (NULL);
    }
    /* create the initial list of nodes */
    hash->nodes = calloc(fc_first_prime_number(), sizeof (FC_HNode*));
    if (hash->nodes == NULL)
    {
        fc_message("cannot allocate nodes list for a hash-table");
        free(hash);
        return (NULL);
    }
    /* create the initial nodes */
    hash->nb_rnodes = 0;
    hash->max_rnodes = 64;
    hash->rnodes = calloc(hash->max_rnodes, sizeof (FC_HNode));
    if (hash->rnodes == NULL)
    {
        fc_message("cannot allocate nodes for a hash-table");
        free(hash->rnodes);
        free(hash);
        return (NULL);
    }
    /* initialize the nodes */
    for (i = 0; i < hash->max_rnodes; i++)
    {
        hash->rnodes[i].fnext = i < hash->max_rnodes - 1 ? &(hash->rnodes[i + 1]) : NULL;
    }
    hash->free_entry = &(hash->rnodes[0]);

    /* initialize the elements */
    hash->size = fc_first_prime_number();
    hash->nb = 0;
    hash->frozen = 0;
    hash->nb_add_rnodes = 0;
    for (i = 0; i < FC_HASH_MAX_ADD; i++)
    {
        hash->add_rnodes[i] = NULL;
    }

    return (hash);
}

/* (!) destroy a hash-table */
void fc_hash_destroy(FC_Hash *hash)
{
    int i;

    if (hash == NULL)
        return;

    if (hash->nodes != NULL)
        free(hash->nodes);
    if (hash->rnodes != NULL)
        free(hash->rnodes);
    for (i = 0; i < hash->nb_add_rnodes; i++)
    {
        if (hash->add_rnodes[i] != NULL)
            free(hash->add_rnodes[i]);
    }
    free(hash);
}

/* (!) found a free entry in the rnode list */
static FC_HNode *fc_hash_get_a_node(FC_Hash *hash)
{
    FC_HNode *tmp;
    int i;

    /* get the next available node */
    tmp = hash->free_entry;
    /* this entry is no more free */
    hash->free_entry = tmp->fnext;

    hash->nb_rnodes++;

    /* no more entry available (after this one) */
    if (hash->free_entry == NULL)
    {/* reallocation of the list */
        /* no more blocks of nodes available */
        if (hash->nb_add_rnodes == FC_HASH_MAX_ADD)
        {/* nax number of node blocks */
            fc_message("no more block of nodes availalbe. Table destroyed.");
            fc_hash_destroy(hash);
            return (NULL);
        }

        /* allocate a new block of nodes */
        hash->add_rnodes[hash->nb_add_rnodes] =
                malloc(sizeof (FC_HNode)*(2 * hash->max_rnodes));
        if (hash->add_rnodes[hash->nb_add_rnodes] == NULL)
        {
            fc_message("cannot reallocate nodes for a hash-table. Table destroyed.");
            fc_hash_destroy(hash);
            return (NULL);
        }
        /* initialize the new list of nodes */
        for (i = 0; i < 2 * hash->max_rnodes; i++)
        {
            hash->add_rnodes[hash->nb_add_rnodes][i].fnext =
                    (i < 2 * hash->max_rnodes - 1) ? &(hash->add_rnodes[hash->nb_add_rnodes][i + 1]) : NULL;
        }
        hash->free_entry = &(hash->add_rnodes[hash->nb_add_rnodes][0]);
        hash->max_rnodes *= 2;
        hash->nb_add_rnodes++;
    }

    return (tmp);
}


/* insert the given node in the temporary list */

/* the field fnext is used to keep the new links */
static void fc_hash_insert_known(FC_HNode **nodes, FC_HNode *nd, int size)
{
    int index;
    FC_HNode *tmp;

    /* compute its new index */
    index = (((unsigned int) (nd->key))) % size;

    nd->next = NULL;
    if (nodes[index] == NULL)
    {
        nodes[index] = nd;
    }
    else
    {
        tmp = nodes[index];
        while (tmp->next != NULL)
        {
            tmp = tmp->next;
        }
        tmp->next = nd;
    }
}

/* (!) increase the size of the hash-table */
static void fc_hash_increase_size(FC_Hash *hash)
{
    FC_HNode **new_nodes = NULL, *tmp = NULL;
    int new_size = 0;
    int i;

    if (hash->frozen)
        return;

    /* to prevent re-calls to this functions */
    hash->frozen = 1;

    new_size = (int) fc_spaced_primes_closest((unsigned int) hash->size);

    /* create the new list */
    new_nodes = calloc(new_size, sizeof (FC_HNode*));

    /* for each existing entry, re-hash it in the new table */
    for (i = 0; i < hash->size; i++)
    {
        tmp = hash->nodes[i];
        while (tmp != NULL)
        {
            /* remove it from the current list */
            hash->nodes[i] = tmp->next;
            /* and re-insert it in the table (without creation) */
            fc_hash_insert_known(new_nodes, tmp, new_size);

            /* agin until this part is empty */
            tmp = hash->nodes[i];
        }
    }
    /* replace the new list */
    hash->nodes = new_nodes;
    hash->size = new_size;

    hash->frozen = 0;
}

/* debug */
void fc_hash_debug(FC_Hash *hash)
{
    int i;
    FC_HNode *tmp;

    if (hash == NULL)
        return;

    for (i = 0; i < hash->size; i++)
    {
        printf("Point %d: [%p]\n", i, hash->nodes[i]);
        tmp = hash->nodes[i];
        while (tmp != NULL)
        {
            printf("k=%p [%p] : ", tmp->key, tmp);
            tmp = tmp->next;
        }
        printf("\n");
    }
}

/* returns the corresp. node if any */
static FC_HNode *fc_hash_lookup_node(FC_Hash *hash, void *key)
{
    int index;
    FC_HNode *tmp;

    if ((hash == NULL) || (key == NULL))
        return (NULL);

    /* compute the hash position */
    index = (((unsigned int) key)) % hash->size;

    tmp = hash->nodes[index];

    while (tmp != NULL)
    {
        if (tmp->key == key)
        {
            return (tmp);
        }
        tmp = tmp->next;
    }
    /* not found */
    return (NULL);
}

/* (!) insert an element in a hash-table */
void fc_hash_insert(FC_Hash *hash, void *key,
                    int val, void *ptr1, void *ptr2)
{
    int index;
    FC_HNode *tmp, *pos;

    if ((hash == NULL) || (key == NULL))
        return;

    /* first try to get the element */
    tmp = fc_hash_lookup_node(hash, key);

    /* still here, just modify it */
    if (tmp != NULL)
    {
        tmp->key = key;
        tmp->value = val;
        tmp->ptr1 = ptr1;
        tmp->ptr2 = ptr2;
        return;
    }


    /* compute the hash position */
    index = (((unsigned int) key)) % hash->size;

    /* get a new node */
    tmp = fc_hash_get_a_node(hash);
    tmp->key = key;
    tmp->value = val;
    tmp->ptr1 = ptr1;
    tmp->ptr2 = ptr2;

    /* insert it */
    tmp->next = NULL;
    if (hash->nodes[index] == NULL)
    {
        hash->nodes[index] = tmp;
    }
    else
    {
        pos = hash->nodes[index];
        while (pos->next != NULL)
        {
            pos = pos->next;
        }
        pos->next = tmp;
    }

    /* one more element */
    hash->nb++;

    /* if the number of element is too high, resize */
    if ((float) hash->nb / hash->size > 0.85)
    {
        fc_hash_increase_size(hash);
    }
}

/* (!) search an element in a hash-table */
int fc_hash_lookup(FC_Hash *hash, void *key,
                   int *val, void **ptr1, void **ptr2)
{
    int index;
    FC_HNode *tmp;

    if ((hash == NULL) || (key == NULL))
        return (0);

    /* compute the hash position */
    index = (((unsigned int) key)) % hash->size;

    tmp = hash->nodes[index];

    while (tmp != NULL)
    {
        if (tmp->key == key)
        {
            *val = tmp->value;
            *ptr1 = tmp->ptr1;
            *ptr2 = tmp->ptr2;
            return (1);
        }
        tmp = tmp->next;
    }

    return (0);
}

/* (!) search an element in a hash-table and return pointers
   on the entries (in order to modify their values without
   having to remove/modify/insert.
   WARNING: these pointers are only valid until you perfom
            an other action on this hash-table. after any
            operation they may become invalid */
int fc_hash_lookup_modify(FC_Hash *hash, void *key,
                          int **val, void ***ptr1, void ***ptr2)
{
    int index;
    FC_HNode *tmp;

    if ((hash == NULL) || (key == NULL))
        return (0);

    /* compute the hash position */
    index = (((unsigned int) key)) % hash->size;

    tmp = hash->nodes[index];

    while (tmp != NULL)
    {
        if (tmp->key == key)
        {
            *val = &(tmp->value);
            *ptr1 = &(tmp->ptr1);
            *ptr2 = &(tmp->ptr2);
            return (1);
        }
        tmp = tmp->next;
    }

    *val = NULL;
    *ptr1 = NULL;
    *ptr2 = NULL;
    return (0);
}

/* (!) remove an element from a hash-table */
void fc_hash_remove(FC_Hash *hash, void *key)
{
    int index;
    FC_HNode *tmp, *otmp;

    if ((hash == NULL) || (key == NULL))
        return;

    /* compute the hash position */
    index = (((unsigned int) key)) % hash->size;

    otmp = NULL;
    tmp = hash->nodes[index];

    while (tmp != NULL)
    {
        if (tmp->key == key)
        {
            /* the old one points to the current next */
            if (otmp != NULL)
            {
                otmp->next = tmp->next;
            }
            else
            {/* the first */
                hash->nodes[index] = tmp->next;
            }
            /* add the node in the free list */
            tmp->fnext = hash->free_entry;
            hash->free_entry = tmp;
            tmp->key = NULL;
            hash->nb--;

            /* done */
            return;
        }
        /* next entry */
        otmp = tmp;
        tmp = tmp->next;
    }

    /* entry not found */
}

/* (!) apply a function to each elements of the hash-table */
void fc_hash_foreach(FC_Hash *hash, FC_HFunc func, void *data)
{
    int i;
    FC_HNode *tmp;

    if ((hash == NULL) || (func == NULL))
        return;

    /* for each list */
    for (i = 0; i < hash->size; i++)
    {
        tmp = hash->nodes[i];
        while (tmp != NULL)
        {
            (func) (tmp->key, tmp->value, tmp->ptr1,
                    tmp->ptr2, data);

            tmp = tmp->next;
        }
    }
}

/* (!) get the number of elements in the hash-table */
int fc_hash_size(FC_Hash *hash)
{
    if (hash == NULL)
        return (0);
    return (hash->nb);
}

