
/**
  @author Ying Zeng (YinzCN_at_Gmail.com)
**/


/*
    lvl_group_t 等级组, 根据大小分级
*/
struct lvl_group_t {
    int lvl_size;  /* block size level, same as blk_size */
    int lasttime;
    int n_blk_groups;
    struct blk_group_t *blk_groups;
    struct lvl_group_t *prev;
    struct lvl_group_t *next;
};


/*
    blk_group_t 小块组
    每小组最多 256 个 mem_block
*/
struct blk_group_t {
    int blk_size;    /* block size level */
    int fullflag;
    int lasttime;
    int n_mem_blocks;
    struct lvl_group_t *lvl_group;    /* 所属等级组 */
    struct mem_block_t *mem_blocks;
    struct blk_group_t *prev;
    struct blk_group_t *next;
};


/*
    mem_block_t 内存块
*/
struct mem_block_t {
    int blk_size;
    int in_use;
    int lasttime;
    struct blk_group_t *blk_group;    /* 所属内存块组 */
    struct mem_block_t *prev;
    struct mem_block_t *next;
    void *memptr;
};


void
free_mem_blocks(struct mem_block_t *mem_blocks)
{
    if (mem_blocks == NULL) {
        return;
    }
    struct mem_block_t *curr, *next, *last;
    curr = mem_blocks;
    last = curr->prev;
    while (1) {
        next = curr->next;
        free(curr);
        if (curr == last) {
            break;
        }
        curr = next;
    }
}


void
free_blk_groups(struct blk_group_t *blk_groups)
{
    if (blk_groups == NULL) {
        return;
    }
    struct blk_group_t *curr, *next, *last;
    curr = blk_groups;
    last = curr->prev;
    while (1) {
        next = curr->next;
        free_mem_blocks(curr->mem_blocks);
        free(curr);
        if (curr == last) {
            break;
        }
        curr = next;
    }
}


void
free_lvl_groups(struct lvl_group_t *lvl_groups)
{
    if (lvl_groups == NULL) {
        return;
    }
    struct lvl_group_t *curr, *next, *last;
    curr = lvl_groups;
    last = curr->prev;
    while (1) {
        next = curr->next;
        free_blk_groups(curr->blk_groups);
        free(curr);
        if (curr == last) {
            break;
        }
        curr = next;
    }
}


/*
    创建新的内存块
*/
struct mem_block_t *
new_mem_block(struct blk_group_t *blk_group)
{
    struct mem_block_t *mem_block;
    mem_block = calloc(1, sizeof(struct mem_block_t) + blk_group->blk_size + 2);
    if (mem_block == NULL) {
        return NULL;
    }
    mem_block->blk_size  = blk_group->blk_size;
    mem_block->in_use    = 1;
    mem_block->blk_group = blk_group;
    if (blk_group->mem_blocks) {
        mem_block->prev = blk_group->mem_blocks->prev;
        mem_block->next = blk_group->mem_blocks;
        mem_block->prev->next = mem_block;
        mem_block->next->prev = mem_block;
    } else {
        mem_block->prev = mem_block;
        mem_block->next = mem_block;
        blk_group->mem_blocks = mem_block;
    }
    mem_block->memptr = (void *)mem_block + sizeof(struct mem_block_t);
    blk_group->n_mem_blocks++;
    return mem_block;
}


/*
    获取可用内存块
*/
struct mem_block_t *
get_mem_block(struct blk_group_t * blk_group)
{
    if (!blk_group->mem_blocks) {
        return new_mem_block(blk_group);
    }
    struct mem_block_t *curr, *last;
    curr = blk_group->mem_blocks;
    last = curr->prev;
    while (1) {
        if (curr->in_use == 0) {
            curr->in_use = 1;
            return curr;
        }
        if (curr == last) {
            if (blk_group->n_mem_blocks > 255) {
                blk_group->fullflag = 1;
            }
            break;
        }
        curr = curr->next;
    }

    if (blk_group->fullflag == 1) {
        blk_group = new_blk_group(blk_group->lvl_group);
    }
    return new_mem_block(blk_group);
}


/*
    创建新的内存块组
*/
struct blk_group_t *
new_blk_group(struct lvl_group_t *lvl_group)
{
    struct blk_group_t *blk_group;
    blk_group = calloc(1, sizeof(struct blk_group_t));
    if (blk_group == NULL) {
        return NULL;
    }
    blk_group->lvl_group = lvl_group;
    blk_group->blk_size  = lvl_group->lvl_size;
    if (lvl_group->blk_groups) {
        blk_group->prev = lvl_group->blk_groups->prev;
        blk_group->next = lvl_group->blk_groups;
        blk_group->prev->next = blk_group;
        blk_group->next->prev = blk_group;
    } else {
        blk_group->prev = blk_group;
        blk_group->next = blk_group;
        lvl_group->blk_groups = blk_group;
    }
    lvl_group->n_blk_groups++;
    return blk_group;
}


/*
    获取可用内存块组
*/
struct blk_group_t *
get_blk_group(struct lvl_group_t *lvl_group)
{
    if (!lvl_group->blk_groups) {
        return new_blk_group(lvl_group);
    }
    struct blk_group_t *curr, *last;
    curr = lvl_group->blk_groups;
    last = curr->prev;
    while (1) {
        if (curr->fullflag == 0) {
            return curr;
        }
        if (curr == last) {
            break;
        }
        curr = curr->next;
    }
    return new_blk_group(lvl_group);
}


/*
    创建新的等级组
*/
struct lvl_group_t *
new_lvl_group(struct memp_t *memp, int lvl_size)
{
    struct lvl_group_t *lvl_group;
    lvl_group = calloc(1, sizeof(struct lvl_group_t));
    if (lvl_group == NULL) {
        return NULL;
    }
    lvl_group->lvl_size = lvl_size;
    if (memp->lvl_groups) {
        lvl_group->prev = memp->lvl_groups->prev;
        lvl_group->next = memp->lvl_groups;
        lvl_group->prev->next = lvl_group;
        lvl_group->next->prev = lvl_group;
    } else {
        lvl_group->prev = lvl_group;
        lvl_group->next = lvl_group;
        memp->lvl_groups = lvl_group;
    }
    return lvl_group;
}


/*
    获取对应等级组
*/
struct lvl_group_t *
get_lvl_group(struct memp_t *memp, int lvl_size)
{
    if (!memp->lvl_groups) {
        return new_lvl_group(memp, lvl_size);
    }
    struct lvl_group_t *curr, *last;
    curr = memp->lvl_groups;
    last = curr->prev;
    while (1) {
        if (curr->lvl_size == lvl_size) {
            return curr;
        }
        if (curr == last) {
            break;
        }
        curr = curr->next;
    }
    return new_lvl_group(memp, lvl_size);
}


#ifndef debug

// 分配内存
void *
mpalloc(struct memp_t *memp, int size)
{
    if (!memp) {
        return NULL;
    }
    int sizx = 8;
    while (sizx < size) { sizx *= 2; }
    struct lvl_group_t *lvl_group;
    lvl_group = get_lvl_group(memp, sizx);
    struct blk_group_t *blk_group;
    blk_group = get_blk_group(lvl_group);
    struct mem_block_t *mem_block;
    mem_block = get_mem_block(blk_group);
    mem_indexes_add(memp, mem_block);
    memset(mem_block->memptr, 0, sizx);
    return mem_block->memptr;
}


void
mpfree(struct memp_t *memp, void *memptr)
{
    if (!memptr) {
        return;
    }
    struct mem_index_t *mem_index;
    mem_index = mem_indexes_get(memp, memptr - sizeof(struct mem_block_t));
    if (!mem_index) {
        htd_log(memp->htdx, "error: call mem_free() with the memptr is not in the indexes.");
        return;
    }
    mem_index->mem_block->in_use = 0;
    mem_index->mem_block->blk_group->fullflag = 0;
    mem_indexes_del(memp, mem_index);
}

#else

// 分配内存
void *
mem_alloc_dbg(struct memp_t *memp, int size, char *file, int line)
{
    if (!memp) {
        return NULL;
    }
    int sizx = 32;
    while (sizx < size) sizx *= 2;

    struct lvl_group_t *lvl_group;
    lvl_group = get_lvl_group(memp, sizx);

    struct blk_group_t *blk_group;
    blk_group = get_blk_group(lvl_group);

    struct mem_block_t *mem_block;
    mem_block = get_mem_block(blk_group);

    mem_indexes_add(memp, mem_block);

    memset(mem_block->memptr, 0, sizx);

    return mem_block->memptr;
}


void
mem_free_dbg(struct memp_t *memp, void *memptr, char *file, int line)
{
    if (!memptr) {
        return;
    }
    struct mem_index_t *mem_index;
    mem_index = mem_indexes_get(memp, memptr - sizeof(struct mem_block_t));
    if (!mem_index) {
        return;
    }

    mem_index->mem_block->in_use = 0;
    mem_index->mem_block->blk_group->fullflag = 0;
    mem_indexes_del(memp, mem_index);
}

#endif

void *
mem_realloc(struct memp_t *memp, void *memptr, int size)
{
    void *newptr = mem_alloc(memp, size);
    memcpy(newptr, memptr, size);
    mem_free(memp, memptr);
    return newptr;
}
