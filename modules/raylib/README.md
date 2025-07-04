# Seal Raylib Binding
## Functions

### Window-related
- [x] init_window(w, h, title)
- [x] close_window()
- [x] window_should_close()
- [ ] set_window_state(flags)

### Cursor-related
- [ ] ShowCursor(void);
- [ ] HideCursor(void);
- [ ] IsCursorHidden(void);
- [ ] EnableCursor(void);
- [ ] DisableCursor(void);
- [ ] IsCursorOnScreen(void);

### Drawing-related
- [x] clear_background(r, g, b)
- [x] begin_drawing()
- [x] end_drawing()
- [ ] BeginMode2D(Camera2D camera);
- [ ] EndMode2D(void);
- [ ] BeginMode3D(Camera3D camera);
- [ ] EndMode3D(void);
- [ ] BeginShaderMode(Shader shader);
- [ ] EndShaderMode(void);
- [ ] BeginScissorMode(int x, int y, int width, int height);
- [ ] EndScissorMode(void);

### Timing-related functions
- [x] set_fps(fps) => SetTargetFPS(int fps);
- [x] delta_time() => GetFrameTime(void);
- [ ] GetTime(void);
- [x] get_fps()

### Input-related functions: keyboard
- [x] is_key_pressed(key)
- [ ] IsKeyPressedRepeat(int key);
- [x] is_key_down(key)
- [x] is_key_released(key)
- [x] is_key_up(key);
- [ ] GetKeyPressed(void);
- [ ] GetCharPressed(void);
- [x] set_exit_key(key)

### Input-related functions: mouse
- [x] is_mouse_pressed(button)
- [x] is_mouse_down(button)
- [x] is_mouse_released(button)
- [x] is_mouse_up(button)
- [x] mouse_x()
- [x] mouse_y()
- [ ] GetMouseDelta(void);
- [ ] SetMousePosition(int x, int y);
- [ ] SetMouseOffset(int offsetX, int offsetY);
- [ ] SetMouseScale(float scaleX, float scaleY);
- [ ] GetMouseWheelMove(void);
- [ ] SetMouseCursor(int cursor);

### Basic shapes drawing functions
- [ ] DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color);                // Draw a line
- [ ] DrawLineV(Vector2 startPos, Vector2 endPos, Color color);                                     // Draw a line (using gl lines)
- [ ] DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color);                       // Draw a line (using triangles/quads)
- [ ] DrawLineStrip(const Vector2 *points, int pointCount, Color color);                            // Draw lines sequence (using gl lines)
- [ ] DrawLineBezier(Vector2 startPos, Vector2 endPos, float thick, Color color);                   // Draw line segment cubic-bezier in-out interpolation
- [ ] DrawCircle(int centerX, int centerY, float radius, Color color);                              // Draw a color-filled circle
- [ ] DrawCircleSector(Vector2 center, float radius, float startAngle, float endAngle, int segments, Color color);      // Draw a piece of a circle
- [ ] DrawCircleSectorLines(Vector2 center, float radius, float startAngle, float endAngle, int segments, Color color); // Draw circle sector outline
- [ ] DrawCircleGradient(int centerX, int centerY, float radius, Color inner, Color outer);         // Draw a gradient-filled circle
- [ ] DrawCircleV(Vector2 center, float radius, Color color);                                       // Draw a color-filled circle (Vector version)
- [ ] DrawCircleLines(int centerX, int centerY, float radius, Color color);                         // Draw circle outline
- [ ] DrawCircleLinesV(Vector2 center, float radius, Color color);                                  // Draw circle outline (Vector version)
- [ ] DrawEllipse(int centerX, int centerY, float radiusH, float radiusV, Color color);             // Draw ellipse
- [ ] DrawEllipseLines(int centerX, int centerY, float radiusH, float radiusV, Color color);        // Draw ellipse outline
- [ ] DrawRing(Vector2 center, float innerRadius, float outerRadius, float startAngle, float endAngle, int segments, Color color); // Draw ring
- [ ] DrawRingLines(Vector2 center, float innerRadius, float outerRadius, float startAngle, float endAngle, int segments, Color color);    // Draw ring outline
- [x] draw_rectangle(x, y, w, h, r, g, b, a)
- [ ] DrawRectangleV(Vector2 position, Vector2 size, Color color);                                  // Draw a color-filled rectangle (Vector version)
- [ ] DrawRectangleRec(Rectangle rec, Color color);                                                 // Draw a color-filled rectangle
- [ ] DrawRectanglePro(Rectangle rec, Vector2 origin, float rotation, Color color);                 // Draw a color-filled rectangle with pro parameters
- [ ] DrawRectangleGradientV(int posX, int posY, int width, int height, Color top, Color bottom);   // Draw a vertical-gradient-filled rectangle
- [ ] DrawRectangleGradientH(int posX, int posY, int width, int height, Color left, Color right);   // Draw a horizontal-gradient-filled rectangle
- [ ] DrawRectangleGradientEx(Rectangle rec, Color topLeft, Color bottomLeft, Color topRight, Color bottomRight); // Draw a gradient-filled rectangle with custom vertex colors
- [ ] DrawRectangleLines(int posX, int posY, int width, int height, Color color);                   // Draw rectangle outline
- [ ] DrawRectangleLinesEx(Rectangle rec, float lineThick, Color color);                            // Draw rectangle outline with extended parameters
- [ ] DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color);              // Draw rectangle with rounded edges
- [ ] DrawRectangleRoundedLines(Rectangle rec, float roundness, int segments, Color color);         // Draw rectangle lines with rounded edges
- [ ] DrawRectangleRoundedLinesEx(Rectangle rec, float roundness, int segments, float lineThick, Color color); // Draw rectangle with rounded edges outline
- [ ] DrawTriangle(Vector2 v1, Vector2 v2, Vector2 v3, Color color);                                // Draw a color-filled triangle (vertex in counter-clockwise order!)
- [ ] DrawTriangleLines(Vector2 v1, Vector2 v2, Vector2 v3, Color color);                           // Draw triangle outline (vertex in counter-clockwise order!)
- [ ] DrawTriangleFan(const Vector2 *points, int pointCount, Color color);                          // Draw a triangle fan defined by points (first vertex is the center)
- [ ] DrawTriangleStrip(const Vector2 *points, int pointCount, Color color);                        // Draw a triangle strip defined by points
- [ ] DrawPoly(Vector2 center, int sides, float radius, float rotation, Color color);               // Draw a regular polygon (Vector version)
- [ ] DrawPolyLines(Vector2 center, int sides, float radius, float rotation, Color color);          // Draw a polygon outline of n sides
- [ ] DrawPolyLinesEx(Vector2 center, int sides, float radius, float rotation, float lineThick, Color color); // Draw a polygon outline of n sides with extended parameters

### Texture-related functions
- [x] load_texture(path)
- [x] unload_texture(tex)
- [ ] draw_texture(tex, x, y, r, g, b, a)
