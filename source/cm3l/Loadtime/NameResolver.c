#include <cm3l/Loadtime/NameResolver.h>
#include <cm3l/Lib/Memory.h>
#include <cm3l/Lib/HashMapEv.h>

#include <string.h>

#define sizeofRefStatM(stat) \
	(sizeof(cm3l_RefStatLinearRepr) + sizeof(cm3l_CodeSelectorPtr) * (stat)->length)

static size_t countSize(cm3l_StatementReference const *refst)
{
	size_t memsize = !!(refst->flags & cm3l_SttRefIsAbsolute);
	if (refst->flags & cm3l_SttRefIsNested) {
		const size_t len = refst->data.nested.length;
		for (size_t i = 0; i != len; ++i) {
			cm3l_CodeSelectorPtr csel = refst->data.nested.parts[i];
			memsize += (i + 1 != len) + csel.end - csel.begin;
		}
	} else {
		cm3l_CodeSelectorPtr csel = refst->data.regular;
		memsize = csel.end - csel.begin;
	}
	return memsize + 1;
}

size_t cm3l_RefStatToLinear(cm3l_StatementReference const *refst, char *buf)
{
	if (buf == NULL)
		return countSize(refst);

	char *iter = buf;
	if (refst->flags & cm3l_SttRefIsAbsolute)
		*iter++ = '!';
	if (refst->flags & cm3l_SttRefIsNested) {
		const size_t len = refst->data.nested.length;
		for (size_t i = 0; i != len; ++i) {
			cm3l_CodeSelectorPtr csel = refst->data.nested.parts[i];
			memcpy(iter, csel.begin, csel.end - csel.begin);
			iter += csel.end - csel.begin;
			if (i + 1 != len)
				*iter++ = ':';
		}
	} else {
		cm3l_CodeSelectorPtr csel = refst->data.regular;
		memcpy(iter, csel.begin, csel.end - csel.begin);
		iter += csel.end - csel.begin;
	}
	*iter++ = 0;
	return iter - buf;
}

int cm3l_ResolveReference (
	cm3l_ParserContext *ctx,
	size_t refId,
	int refType
) {
	cm3l_Vector *vnr = &ctx->vnameRes;
	cm3l_Vector *vref = &ctx->outp->references;
	if (!vnr->length) return 1;

	cm3l_StatementReference *refst = cm3l_VectorAtM(vref, refId, cm3l_StatementReference);

	size_t refstrlen = cm3l_RefStatToLinear(refst, NULL);
	char *refstr;
	assert(refstrlen);
	refstr = malloc(refstrlen);
	cm3l_RefStatToLinear(refst, refstr);

	// the lookup is performed from the topmost locals context
	// to the downmost globals context
	if (refType == 1)
	{
		refst->id = CM3L_NONE;
		for (size_t i = vnr->length - 1;; --i)
		{
			cm3l_NameResolver *resolver = cm3l_VectorAtM(vnr, i, cm3l_NameResolver);
			size_t *val = cm3l_HashMapEvAt(&resolver->scope, refstr, refstrlen);
			if (val != NULL) {
				refst->id = *val;
				assert(refst->id != CM3L_NONE);
				break;
			}
			if (!i) break;
		}
		free(refstr);
		return (refst->id == CM3L_NONE);
	}

	cm3l_NameResolver *resolver = cm3l_VectorBackM(vnr, cm3l_NameResolver);
	size_t *val = cm3l_HashMapEvAt(&resolver->scope, refstr, refstrlen);
	if (val != NULL) {
		free(refstr);
		return 2;
	}

	cm3l_HashMapEvInsertKV (
		&resolver->scope,
		refstr,
		refstrlen,
		&resolver->nextId,
		sizeof(size_t)
	);
	free(refstr);

	refst->id = resolver->nextId++;
	return 0;
}
