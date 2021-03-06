/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010-2014 Facebook, Inc. (http://www.facebook.com)     |
   | Copyright (c) 1997-2010 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#ifndef incl_HPHP_EXT_CONTINUATION_H_
#define incl_HPHP_EXT_CONTINUATION_H_


#include "hphp/runtime/base/base-includes.h"
#include "hphp/runtime/vm/resumable.h"
#include "hphp/system/systemlib.h"

typedef unsigned char* TCA; // "Translation cache address."

namespace HPHP {
///////////////////////////////////////////////////////////////////////////////

FORWARD_DECLARE_CLASS(Continuation);

///////////////////////////////////////////////////////////////////////////////
// class Continuation

struct c_Continuation : ExtObjectDataFlags<ObjectData::HasClone> {
  DECLARE_CLASS_NO_SWEEP(Continuation)

  static constexpr ptrdiff_t resumableOff() { return -sizeof(Resumable); }
  static constexpr ptrdiff_t arOff() {
    return resumableOff() + Resumable::arOff();
  }
  static constexpr ptrdiff_t offsetOff() {
    return resumableOff() + Resumable::offsetOff();
  }
  static constexpr ptrdiff_t startedOff() {
    return offsetof(c_Continuation, o_subclassData.u8[0]);
  }
  static constexpr ptrdiff_t stateOff() {
    return offsetof(c_Continuation, o_subclassData.u8[1]);
  }

  bool started() const { return o_subclassData.u8[0]; }
  void start() { o_subclassData.u8[0] = true; }

  enum ContState : uint8_t {
    Running = 1,
    Done    = 2
  };

  bool done() const { return o_subclassData.u8[1] & ContState::Done; }
  void setDone() { o_subclassData.u8[1]  =  ContState::Done; }

  bool running() const { return o_subclassData.u8[1] & ContState::Running; }
  void setRunning() { o_subclassData.u8[1]  =  ContState::Running; }
  void setStopped() { o_subclassData.u8[1] &= ~ContState::Running; }

  void t___construct();
  void suspend(Offset offset, const Cell& value);
  void suspend(Offset offset, const Cell& value, const Cell& key);
  Object t_getwaithandle();
  Variant t_current();
  Variant t_key();
  void t_next();
  void t_rewind();
  bool t_valid();
  void t_send(const Variant& v);
  void t_raise(const Variant& v);
  String t_getorigfuncname();
  String t_getcalledclass();

  static c_Continuation* Clone(ObjectData* obj);

  static c_Continuation* Create(const ActRec* fp, Offset offset) {
    assert(fp);
    assert(fp->func()->isGenerator());
    void* obj = Resumable::Create(fp, offset, sizeof(c_Continuation));
    auto const cont = new (obj) c_Continuation();
    cont->incRefCount();
    cont->setNoDestruct();
    return cont;
  }

  /**
   * Get adjusted generator function base() where the real user code starts.
   *
   * Skips CreateCont and PopC opcodes.
   */
  static Offset userBase(const Func* func) {
    assert(func->isGenerator());
    auto base = func->base();

    DEBUG_ONLY auto op = reinterpret_cast<const Op*>(func->unit()->at(base));
    assert(op[0] == OpCreateCont);
    assert(op[1] == OpPopC);

    return base + 2;
  }

  inline void preNext() {
    if (done()) {
      throw_exception(Object(SystemLib::AllocExceptionObject(
                               "Continuation is already finished")));
    }
    if (running()) {
      throw_exception(Object(SystemLib::AllocExceptionObject(
                               "Continuation is already running")));
    }
    setRunning();
    start();
  }

  inline void startedCheck() {
    if (!started()) {
      throw_exception(
        Object(SystemLib::AllocExceptionObject("Need to call next() first")));
    }
  }

private:
  explicit c_Continuation(Class* cls = c_Continuation::classof());
  ~c_Continuation();

  void copyContinuationVars(ActRec *fp);

public:
  int64_t m_index;
  Cell m_key;
  Cell m_value;

  Resumable* resumable() const {
    return reinterpret_cast<Resumable*>(
      const_cast<char*>(reinterpret_cast<const char*>(this) + resumableOff()));
  }

  ActRec* actRec() const {
    return resumable()->actRec();
  }
};

///////////////////////////////////////////////////////////////////////////////
}

#endif // incl_HPHP_EXT_CONTINUATION_H_
