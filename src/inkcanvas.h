#pragma once

#include "stroke.h"

#include <QImage>
#include <QQuickPaintedItem>

// Pen surface tuned for e-ink: black-only strokes, incremental painting of new
// segments (small dirty rects -> fast partial refresh) and no animations.
// Input arrives from QML (PointHandler) so pressure and eraser tip are known.
class InkCanvas : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(qreal penWidth READ penWidth WRITE setPenWidth NOTIFY penWidthChanged)
    Q_PROPERTY(bool empty READ isEmpty NOTIFY strokesChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY strokesChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY strokesChanged)
    Q_PROPERTY(bool modified READ isModified NOTIFY modifiedChanged)
    // Paper like the reMarkable templates: "lined", "grid", "dots" or "blank".
    Q_PROPERTY(QString paperTemplate READ paperTemplate WRITE setPaperTemplate NOTIFY paperChanged)
    Q_PROPERTY(qreal lineSpacing READ lineSpacing WRITE setLineSpacing NOTIFY paperChanged)

public:
    explicit InkCanvas(QQuickItem *parent = nullptr);

    QString source() const { return m_source; }
    void setSource(const QString &path);
    qreal penWidth() const { return m_penWidth; }
    void setPenWidth(qreal w);
    bool isEmpty() const { return m_strokes.isEmpty(); }
    bool canUndo() const { return !m_undo.isEmpty(); }
    bool canRedo() const { return !m_redo.isEmpty(); }
    bool isModified() const { return m_modified; }
    QString paperTemplate() const { return m_template; }
    void setPaperTemplate(const QString &t);
    qreal lineSpacing() const { return m_spacing; }
    void setLineSpacing(qreal s);

    Q_INVOKABLE void beginStroke(qreal x, qreal y, qreal pressure);
    Q_INVOKABLE void extendStroke(qreal x, qreal y, qreal pressure);
    Q_INVOKABLE void endStroke();
    Q_INVOKABLE void eraseAt(qreal x, qreal y, qreal radius = 12);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE bool save();
    Q_INVOKABLE void reload();

    const QVector<Stroke> &strokes() const { return m_strokes; }
    const QImage &buffer() const { return m_buffer; }
    QRect lastRepaint() const { return m_lastRepaint; } // for tests
    QImage renderAll() const; // all strokes painted from scratch

    // Monochrome ink (no antialiasing): only black and transparent pixels, so
    // the e-ink panel never needs a grey-level refresh for ink. On by default
    // on the device (main.cpp), off on desktop.
    static void setMonochrome(bool on) { s_monochrome = on; }
    static bool monochrome() { return s_monochrome; }

    void paint(QPainter *painter) override;

signals:
    void sourceChanged();
    void penWidthChanged();
    void strokesChanged();
    void modifiedChanged();
    void paperChanged();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    void rebuildBuffer();
    // Undo, redo and the eraser only repaint where strokes changed: a full
    // repaint took ~250 µs per point on the rM1 and refreshed the whole panel
    // (#12). Strokes are compared by their shared point data.
    void showStrokes(const QVector<Stroke> &next);
    void repaintRect(const QRect &r);
    QRect strokeRect(const Stroke &s) const;
    void pushUndo();
    void setModified(bool m);
    QPointF norm(qreal x, qreal y) const;

    QString m_source;
    qreal m_penWidth = 2.5;
    QVector<Stroke> m_strokes;
    QVector<QVector<Stroke>> m_undo;
    QVector<QVector<Stroke>> m_redo;
    bool m_drawing = false;
    bool m_modified = false;
    QImage m_buffer;
    QRect m_lastRepaint;
    static inline bool s_monochrome = false;
    QString m_template = QStringLiteral("blank");
    qreal m_spacing = 0;
};
