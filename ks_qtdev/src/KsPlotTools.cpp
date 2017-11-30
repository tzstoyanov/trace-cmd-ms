#include <QMouseEvent>

#include <iostream>
#include <GL/glut.h>
#include <GL/gl.h>

#include "KsPlotTools.h"

namespace KsPlot {

Color::Color()
: r_(0.f), g_(0.f), b_(0.1f)
{}

Color::Color(float r, float g, float b)
: r_(r), g_(g), b_(b)
{}

Color& Color::operator ++()
{
	if (r_<.7f)
		r_ += .2f;
	else if ( g_ < .75f)
		g_ += .2f;
	else if (b_ < .7f)
		b_ += .2f;
	else {
		r_ = 0.f;
		g_ = 0.1f;
		b_ = 0.2f;
	}

	return *this;
}

Point::Point()
: x_(0), y_(0)
{}

Point::Point(int x, int y)
: x_(x), y_(y)
{}

void Point::reset()
{
	x_ = 0;
	y_ = 0;
}

void Point::draw() const
{
	glBegin(GL_POINTS);
	glColor3f(0.0f,0.0f,0.0f);	// set the drawing color (black)
	glVertex2f(x_, y_);
	glEnd();
}

void Point::draw(const Color &c) const
{
	glBegin(GL_POINTS);
	glColor3f(c.r_, c.g_, c.b_);
	glVertex2i(x_, y_);
	glEnd();
}


void Point::print() const {std::cout << "(" << x_ << ", " << y_ << ")\n";}

Line::Line()
: a_(NULL), b_(NULL)
{}

Line::Line(Point *a, Point *b)
: a_(a), b_(b)
{}

void Line::draw() const
{
	glBegin(GL_LINES);
	glColor3f(0.0f,0.0f,0.0f);                      // set the drawing color(black)
	glVertex2i(this->a_->x_, this->a_->y_);
	glVertex2i(this->b_->x_, this->b_->y_);
	glEnd();
}

void Line::draw(const Color &c) const
{
	glBegin(GL_LINES);
	glColor3f(c.r_, c.g_, c.b_);
	glVertex2i(this->a_->x_, this->a_->y_);
	glVertex2i(this->b_->x_, this->b_->y_);
	glEnd();
}

Bin::Bin()
: id_(0), color_(0.f, 0.f, 0.f)
{}

Bin::Bin(size_t id)
: id_(id), color_(0.f, 0.f, 0.f)
{}

void Bin::drawLine()
{
	Line l(&base_, &val_);
	l.draw(color_);
}

void Bin::drawVal()
{
	val_.draw();
}

Polygon::Polygon(size_t id, size_t n)
: n_points_(n), points_(new Point*[n]), lines_(new Line*[n])
{}

Polygon::~Polygon()
{
	for(size_t i = 0; i < n_points_; ++i) {
		delete points_[i];
		delete lines_[i];
	}

	delete[] points_;
	delete[] lines_;
}

void Polygon::set_point(size_t i, int x, int y)
{
	points_[i] = new Point(x, y);
}

void Polygon::set_point(size_t i, const Point &p)
{
	this->set_point(i, p.x_, p.y_);
}

void Polygon::make_lines() {
	lines_[0] = new Line;

	lines_[0]->a_ = points_[n_points_ - 1];
	lines_[0]->b_ = points_[0];

	for(size_t i = 1; i < n_points_; ++i) {
		lines_[i] = new Line;
		lines_[i]->a_ = points_[i - 1];
		lines_[i]->b_ = points_[i];
	}
}

Graph::Graph(size_t s)
: bins_(new Bin[s]), size_(s), hMargin_(10) , vMargin_(10)
{
	for (size_t i = 0; i < size_; ++i) {
		bins_[i].base_.x_ = i + hMargin_;
		bins_[i].base_.y_ = vMargin_;
		bins_[i].val_.x_ = bins_[i].base_.x_;
		bins_[i].val_.y_ = bins_[i].base_.y_;
	}
}

Graph::~Graph()
{
	delete[] bins_;
}

size_t Graph::size()
{
	return size_;
}

void Graph::setBase(int b)
{
	for (size_t i = 0; i < size_; ++i)
		bins_[i].base_.y_ = b;
}

void Graph::setBinValue(size_t bin, int val)
{
	bins_[bin].val_.y_ = bins_[bin].base_.y_ + val;
}

void Graph::setBinColor(size_t bin, const Color &c)
{
	bins_[bin].color_ = c;
}

void Graph::draw()
{
	for (size_t i = 0; i < size_; ++i)
		bins_[i].drawLine();
}

}; // KsPlot

KsGLWidget::KsGLWidget(QWidget *parent)
: QOpenGLWidget(parent)
{
	setMouseTracking(true);
}

void KsGLWidget::initializeGL()
{
	printf("initializeGL \n");
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_COLOR_MATERIAL);
	glEnable(GL_BLEND);
	glEnable(GL_POLYGON_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
// 	glLineWidth(1.5);
	glClearColor(1, 1, 1, 1);
}

void KsGLWidget::resizeGL(int w, int h)
{
	printf("resizeGL w: %i  h: %i \n", w, h);
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, w, 0, h); // set origin to bottom left corner
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void KsGLWidget::paintGL() {
	printf("paintGL \n");
	glClear(GL_COLOR_BUFFER_BIT);
	
	for (auto const &g: _graphs)
		g->draw();
}
