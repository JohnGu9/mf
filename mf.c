#include "mf.h"

#include <string.h>

enum {
  NONE,
  CHANGING,
};

#define MemoryAllocFailedReason "Memory alloc failed. "
#define FunctionCallRecursivelyReason                                          \
  "Function [m_attach|m_detach|init_state|m_set_state|dispose] can not "       \
  "be called recursively. "

const mWidget_t NullWidget = {
    .class = NULL,
    .data = NULL,
};

static void mask_context_is_changing(mContext_t *context) {
  if (context->status == CHANGING) {
    context->on_error(FunctionCallRecursivelyReason);
  }

  context->status = CHANGING;
}

static void clear_mask_context_is_changing(mContext_t *context){
  context->status = NONE;
}

void m_attach(mContext_t *context, const mWidget_t *widget,
              void *new_delete_context,
              mContext_t *(*new)(void *new_delete_context),
              void (*delete)(void *new_delete_context, mContext_t *),
              void (*on_error)(const char *reason)) {
  memset(context->children, 0, sizeof(context->children));
  memset(context->children_context, 0, sizeof(context->children_context));

  context->new_delete_context = new_delete_context;
  context->new = new;
  context->delete = delete;
  context->on_error = on_error;
  context->widget = widget;

  context->status = CHANGING;
  context->state = context->widget->class->init_state(context);
  context->widget->class->build(context, context->children);
  for (size_t i = 0; i < MF_MAX_CHILDREN; i++) {
    mWidget_t *const child = &context->children[i];
    if (child->class != NULL) {
      mContext_t *const child_context = new(new_delete_context);
      if (child_context != NULL) {
        context->children_context[i] = child_context;
        child_context->index = i;
        child_context->parent = context;
        m_attach(child_context, child, new_delete_context, new, delete,
                 on_error);
      } else {
        on_error(MemoryAllocFailedReason);
      }
    }
  }
  context->status = NONE;
}

static void m_detach_internal(mContext_t *parent_context, size_t child_index) {
  mContext_t *child_context = parent_context->children_context[child_index];
  if (child_context != NULL) {
    mask_context_is_changing(child_context);
    for (size_t i = MF_MAX_CHILDREN; i != 0; i--) {
      m_detach_internal(child_context, i - 1);
    }
    child_context->widget->class->dispose(child_context);
    parent_context->delete(parent_context->new_delete_context, child_context);
  }
  parent_context->children[child_index].class = NULL;
  parent_context->children[child_index].data = NULL;
  parent_context->children_context[child_index] = NULL;
}

void m_set_state(mContext_t *context) {
  mWidget_t new_children[MF_MAX_CHILDREN] = {0};

  mask_context_is_changing(context);
  context->widget->class->build(context, new_children);
  for (size_t i = 0; i < MF_MAX_CHILDREN; i++) {
    mWidget_t *const child = &context->children[i];
    mWidget_t *const new_child = &new_children[i];
    if (child->class != new_child->class) {
      if (child->class != NULL) {
        m_detach_internal(context, i);
      }
      if (new_child->class != NULL) {
        mContext_t *const child_context = context->new(context->new_delete_context);
        if (child_context != NULL) {
          *child = *new_child;
          context->children_context[i] = child_context;
          child_context->index = i;
          child_context->parent = context;
          m_attach(child_context, child, context->new_delete_context,
                   context->new, context->delete, context->on_error);
        } else {
          context->on_error(MemoryAllocFailedReason);
        }
      }
    } else {
      mContext_t *const child_context = context->children_context[i];
      if (child_context != NULL && new_child->class->should_rebuild(child_context) == m_should_rebuild) {
        m_set_state(child_context);
      }
    }
  }
  clear_mask_context_is_changing(context);
}

void m_detach(mContext_t *context) {
  mask_context_is_changing(context);
  for (size_t i = MF_MAX_CHILDREN; i != 0; i--) {
    m_detach_internal(context, i - 1);
  }
  context->widget->class->dispose(context);
  clear_mask_context_is_changing(context);
}

void *m_get_state(const mContext_t *context) { return context->state; }

const mWidget_t *m_get_widget(const mContext_t *context) {
  return context->widget;
}

const void *m_get_widget_data(const mContext_t *context) {
  return context->widget->data;
}

void *
m_get_state_from_inherited_widget_class(const mContext_t *context,
                                        const mWidgetClass_t *widget_class) {
  for (const mContext_t *parent = context->parent; parent != NULL;
       parent = parent->parent) {
    if (m_get_widget(parent)->class == widget_class) {
      return m_get_state(parent);
    }
  }
  return NULL;
}

mShouldRebuildFlag_t m_always_rebuild(mContext_t *context){
  return m_should_rebuild;
}
