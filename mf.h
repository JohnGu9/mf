#ifndef MF_H
#define MF_H

#ifdef MF_CONFIG
#include "mf-config.h"
#endif

#include <string.h>

#ifndef MF_MAX_CHILDREN
#define MF_MAX_CHILDREN 4
#endif

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @brief
   *
   * @param class: nullable
   * @param key: nullable
   * @param data: nullable
   */
  typedef struct _mWidget mWidget_t;

  /**
   * @brief
   *
   * @param init_state: not nullable
   * @param build: not nullable
   * @param should_rebuild: not nullable
   * @param dispose: not nullable
   */
  typedef struct _mWidgetClass mWidgetClass_t;

  /**
   * @brief Context is for the internal usage. User should not access the internal member.
   *
   */
  typedef struct _mContext mContext_t;

  typedef enum
  {
    m_should_not_rebuild,
    m_should_rebuild,
  } mShouldRebuildFlag_t;

  struct _mWidget
  {
    const mWidgetClass_t *class;
    void *key;
    void *data;
  };

  struct _mWidgetClass
  {
    void *(*const init_state)(mContext_t *context);
    void (*const build)(mContext_t *context, mWidget_t children[MF_MAX_CHILDREN]);
    mShouldRebuildFlag_t (*const should_rebuild)(mContext_t *context);
    void (*const dispose)(mContext_t *context);
  };

  struct _mContext
  {
    // internal use
    // user should not access members directly
    void *new_delete_context;
    mContext_t *(*new)(void *new_delete_context);
    void (*delete)(void *new_delete_context, mContext_t *);
    void (*on_error)(const char *reason);
    const mWidget_t *widget;
    void *state;
    const mContext_t *parent;
    size_t index;
    mWidget_t children[MF_MAX_CHILDREN];
    mContext_t *children_context[MF_MAX_CHILDREN];
    int status;
  };

  extern const mWidget_t NullWidget;

  // top level functions

  /**
   * @brief create a widget tree from the root widget and attach it to the root context
   *
   * @param context: not nullable
   * @param widget: not nullable
   * @param new_delete_context: nullable
   * @param new: not nullable
   * @param delete: not nullable
   * @param on_error: not nullable
   *
   */
  void m_attach(mContext_t *context /* not nullable */,
                const mWidget_t *widget /* not nullable */,
                void *new_delete_context /* nullable */,
                mContext_t *(*new)(void *new_delete_context) /* not nullable */,
                void (*delete)(void *new_delete_context, mContext_t *) /* not nullable */,
                void (*on_error)(const char *reason) /* not nullable */);

  /**
   * @brief destroy the widget that attached to the root context
   *
   * @param context: not nullable
   *
   */
  void m_detach(mContext_t *context);

  // helper functions

  mShouldRebuildFlag_t m_always_rebuild(mContext_t *context);

  /**
   * @brief invoke framework to rebuild the sub widget tree that attached the context
   *
   * @param context: not nullable
   */
  void m_set_state(mContext_t *context);

  void *m_get_state(const mContext_t *context);

  const mWidget_t *m_get_widget(const mContext_t *context);

  const void *m_get_widget_data(const mContext_t *context);

  void *
  m_get_state_from_inherited_widget_class(const mContext_t *context,
                                          const mWidgetClass_t *widget_class);

#ifndef m_get_state_cast
#define m_get_state_cast(state, context, T) \
  T *const state = (T *)m_get_state((context))
#endif

#ifndef m_get_widget_data_cast
#define m_get_widget_data_cast(data, context, T) \
  const T *const data = (const T *)m_get_widget_data((context))
#endif

#ifndef m_get_state_from_inherited_widget_class_cast
#define m_get_state_from_inherited_widget_class_cast(inherited_widget_state,   \
                                                     context, widget_class, T) \
  T *const inherited_widget_state =                                            \
      (T *)m_get_state_from_inherited_widget_class((context),                  \
                                                   (widget_class))
#endif

#ifdef __cplusplus
}
#endif

#endif
