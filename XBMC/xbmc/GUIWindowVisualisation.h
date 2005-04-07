#pragma once

#include "GUIWindow.h"
#include "GUIVisualisationControl.h"

class CGUIWindowVisualisation :
      public CGUIWindow
{
public:
  CGUIWindowVisualisation(void);
  virtual ~CGUIWindowVisualisation(void);
  virtual void FreeResources();
  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnAction(const CAction &action);
  virtual void OnMouse();
  virtual void Render();
  virtual void OnWindowLoaded();
};
