#ifndef _GLWIDGET_H
#define _GLWIDGET_H

#include <QOpenGLWidget>

namespace KsPlot {

struct Color
{
	Color();
	Color(float r, float g, float b);

	Color& operator ++();

	float r_, g_, b_;
};

class Point
{
public:
	Point();
	Point(int x, int y);

	void reset();
	void draw() const;
	void draw(const Color &c) const;

	/** Cartesian coordinates */
	int x_, y_;

	void print() const;
};

class Line
{
public:
	Line();
	Line(Point *a, Point *b);

	void draw() const;
	void draw(const Color &c) const;

	Point *a_, *b_;
};

class Polygon
{
	Polygon() = delete;

public:
	Polygon(size_t id, size_t n);
	virtual ~Polygon();

	void set_point(size_t i, int x, int y);
	void set_point(size_t i, const Point &p);
	void order();
	void make_lines();

	void draw() const;
	void draw(const Color &c) const;

	size_t	  n_points_;
	Point	**points_;
	Line	**lines_;
};

class Triangle : public Polygon
{
public:
	Triangle(size_t id) : Polygon(id, 3) {}
};

class Rectangle : public Polygon
{
public:
	Rectangle(size_t id) : Polygon(id, 3) {}
};

class Bin {
public:
	Bin();
	Bin(size_t id);

	void drawLine();
	void drawVal();

	size_t	id_;
	Color	color_;
	Point	base_, val_;
};

class Graph {
	Graph() = delete;
public:
	Graph(size_t s);
	~Graph();

	size_t size();
	void setBase(int b);
	void setBinValue(size_t bin, int val);
	void setBinColor(size_t bin, const Color &c);

	void draw();

private:
	Bin	*bins_;
	size_t	 size_;
	int	 hMargin_, vMargin_;
};

}; // KsPlot

class KsGLWidget : public QOpenGLWidget
{
	Q_OBJECT
public:
	KsGLWidget(QWidget *parent = NULL);
// 	virtual ~KsGLWidget() {}

protected:
	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;

	void mousePressEvent(QMouseEvent *event) {}
	void mouseMoveEvent(QMouseEvent *event) {}
	void keyPressEvent(QKeyEvent *event) {}

public:
	QVector<KsPlot::Graph*> _graphs;
};

#endif  /* _GLWIDGET_H */
