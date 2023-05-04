;;;; SPDX-FileCopyrightText: Atlas Engineer LLC
;;;; SPDX-License-Identifier: BSD-3-Clause

(nyxt:define-package :nyxt/renderer/electron
    (:documentation "Electron renderer."))
(in-package :nyxt/renderer/electron)

(push :nyxt-electron *features*)

(define-class electron-renderer (renderer)
  ((name "electron"))
  (:export-class-name-p t)
  (:export-accessor-names-p t))

(setf nyxt::*renderer* (make-instance 'electron-renderer))

(define-class electron-browser ()
  ())

(defmethod install ((renderer electron-renderer))
  (flet ((set-superclasses (renderer-class-sym+superclasses)
           (closer-mop:ensure-finalized
            (closer-mop:ensure-class (first renderer-class-sym+superclasses)
                                     :direct-superclasses (rest renderer-class-sym+superclasses)
                                     :metaclass 'interface-class))))
    (mapc #'set-superclasses '((renderer-browser electron-browser)))))
