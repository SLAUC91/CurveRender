#include <GL/freeglut.h>
#include <vector>
#include <math.h>
#include <algorithm>
using namespace std;
#define M_PI 3.14159

int SCREEN_HEIGHT = 480;

static struct {
	// current parameters for controlling glulookat
	double alpha, beta, dist;
	// viewing mode state (depending on which mouse button is being used)
	enum { NONE, ROTATING, ZOOMING } viewingMode;
	// last mouse position
	int mouseX, mouseY;
	// current recursion for drawing the spheres
	int recursion;
	//flat or Goraud shading
	bool flat;
	// Curve selector 0 - 2
	int CurveType;
	//ObjectType - Wire or Solid
	int ObjectType;
	//courseness degree
	double degree;
} globals;

class Point {
public:
	// constructors
	Point() : x_(0), y_(0), z_(0) {};
	// z = 0 by default,to make it easier to use as 2D point
	Point(double px, double py, double pz = 0.0) : x_(px), y_(py), z_(pz) {}
	// getters
	const double & x() const { return x_; }
	const double & y() const { return y_; }
	const double & z() const { return z_; }
	// setters
	double & x() { return x_; }
	double & y() { return y_; }
	double & z() { return z_; }
	// dot product with another vector
	double dot(const Point & p) const {
		return x_ * p.x() + y_ * p.y() + z_ * p.z();
	}
	// cross product with another vector
	Point cross(const Point & p) const {
		return Point(y_ * p.z() - z_ * p.y(),
			z_ * p.x() - x_ * p.z(),
			x_ * p.y() - y_ * p.x());
	}
	// returns a unit vector of this vector
	Point unit(void) const {
		double lenSq = dot(*this);
		double len = sqrt(lenSq);
		double lenInv = 1 / len;
		return Point(lenInv * x_, lenInv * y_, lenInv * z_);
	}
	// convenience function to draw this as a point using OpenGL
	void glv(void) const { glVertex3d(x_, y_, z_); }
	// convenience function to draw this as a normal
	void gln(void) const { glNormal3d(x_, y_, z_); }

private:
	double x_, y_, z_;
};

// addition
Point operator + (const Point & p1, const Point & p2) {
	return Point(p1.x() + p2.x(), p1.y() + p2.y(), p1.z() + p2.z());
}

// subtraction
Point operator - (const Point & p1, const Point & p2) {
	return Point(p1.x() - p2.x(), p1.y() - p2.y(), p1.z() + p2.z());
}

// unary minus
Point operator - (const Point & p1) {
	return Point(-p1.x(), -p1.y(), -p1.z());
}

// scalar product
Point operator * (double a, const Point & p) {
	return Point(a * p.x(), a * p.y(), a * p.z());
}

std::vector<Point> ControlPoints;	//Original
std::vector<Point> OpenCurve;		//Open ski
std::vector<Point> ClosedCurve;		//Closed

void drawDot(int x, int y) {
	glBegin(GL_POINTS);
	glVertex2i(x, y);
	glEnd();
}

void drawLine(Point p1, Point p2) {
	glBegin(GL_LINES);
	glVertex2f(p1.x(), p1.y());
	glVertex2f(p2.x(), p2.y());
	glEnd();
}

Point computeDeCasteljau(std::vector<Point> p, double t){
	for (unsigned int i = 1; i < p.size(); i++) {
		for (unsigned int j = 0; j < p.size() - i; j++)
			p[j] = (1 - t)*p[j] + t*p[j + 1];
	}
	return p[0];
}

//Compute new open curve control points
std::vector<Point>  chai(std::vector<Point> Points) {
	std::vector<Point> NewPoints;

	// keep the first point
	NewPoints.push_back(Points[0]);
	for (unsigned int i = 0; i<(Points.size()-1); ++i) {

		// get 2 original points
		Point p0 = Points[i];
		Point p1 = Points[i + 1];
		Point Q;
		Point R;

		Q = (0.75*p0 + 0.25*p1);
		R = (0.25*p0 + 0.75*p1);

		NewPoints.push_back(Q);
		NewPoints.push_back(R);
	}

	// keep the last point
	NewPoints.push_back(Points[Points.size() - 1]);

	// update the points array
	return NewPoints;

}

//Reverse open curve control points
std::vector<Point> unchai(std::vector<Point> Points) {
	std::vector<Point> NewPoints;

	// keep the first point
	NewPoints.push_back(Points[0]);
	// step over every 2 points
	for (unsigned int i = 1; i<(Points.size()-1); i += 2) {

		// get last point found in reduced array
		Point last = NewPoints[NewPoints.size() - 1];

		// get 2 original point
		Point p0 = Points[i];
		Point Q;
		Q = p0 - 0.75*last;
		Q = 4.0*Q;

		// add to new curve
		NewPoints.push_back(Q);
	}

	return NewPoints;
}

void myMouse(int button, int state, int x, int y) {
	static bool flag = true;
	static int NUMPOINTS;

	// If left button was clicked
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		// Store where the user clicked, note Y is backwards.
		if (NUMPOINTS < 3){
			Point P1 = Point((float)x, (float)(SCREEN_HEIGHT - y), 0);
			ControlPoints.push_back(P1);
		}

		//Loop over ControlPoints vector
		if (NUMPOINTS >= 3) {
			Point P1 = Point((float)x, (float)(SCREEN_HEIGHT - y), 0);

			//search
			for (int z = 0; z < NUMPOINTS; z++){
				//check if any value is near current x and y
				Point P2 = ControlPoints[z];
				if (P2.x() <= (P1.x() + 30.0) && P2.x() >= (P1.x() - 30.0) && P2.y() <= (P1.y() + 30.0) && P2.y() >= (P1.y() - 30.0) && NUMPOINTS >= 3){
					ControlPoints.erase(ControlPoints.begin() + z);
					flag = false;
					NUMPOINTS -= 2;
					break;
				}
			}

			//if nothing removed
			if (flag == true){
				ControlPoints.push_back(P1);
			}
			flag = true;
		}

		OpenCurve = ControlPoints;
		ClosedCurve = ControlPoints;
		NUMPOINTS++;
		glutPostRedisplay();
	}
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
		exit(0);
	}
}

//compute closed curve
std::vector<Point> closed(std::vector<Point> &a){
	std::vector<Point> NewPoints;
	int N = a.size();
	NewPoints.push_back(0.125*a[N - 1] + 0.75 *a[0] + 0.125*a[1]);
	NewPoints.push_back(0.5*a[0] + 0.5*a[1]);

	for (int i = 1; i <= N - 2; i++) {
		NewPoints.push_back(0.125*a[i - 1] + 0.75 * a[i] + 0.125*a[i + 1]);
		NewPoints.push_back(0.5*a[i] + 0.5*a[i + 1]);
	}

	NewPoints.push_back(0.125*a[N - 2] + 0.75 * a[N - 1] + 0.125*a[0]);
	NewPoints.push_back(0.5 * a[N - 1] + 0.5 * a[0]);

	return NewPoints;
}

//Draw control
void drawcontrol(std::vector<Point> const &q0){
	if (q0.size() >= 1){
		for (int i = 0; i<q0.size(); i++){
			Point AB = q0[i];
			int x = AB.x();
			int y = AB.y();
			drawDot(x, y);
		}
	}
}

//draw BezierCurve
std::vector<Point> drawBezierCurve(std::vector<Point> &q1){
	std::vector<Point> NewPoints;
	if (q1.size() >= 3){
		Point A = q1[0];
		for (double t = 0.0; t <= 1.0; t += 0.1) {
			Point B = computeDeCasteljau(q1, t);
			drawLine(A, B);
			//Points0.push_back(B);
			NewPoints.push_back(B);
			A = B;
		}
	}
	return NewPoints;
}

void OnKeyPress(unsigned char key, int, int) {
	static int LineDefinition = 0; //Definition of the line selector
	switch (key) {
	case ' ':

		if (globals.CurveType == 0)
			globals.CurveType++;
		else if (globals.CurveType == 1)
			globals.CurveType++;
		else if (globals.CurveType == 2)
			globals.CurveType = 0;
		break;

		// increase the LOD
	case '+':
		if (globals.CurveType == 0){
			if (LineDefinition == 0){
				OpenCurve = ControlPoints;
				LineDefinition++;
			}
			else if (LineDefinition == 1){
				OpenCurve = chai(ControlPoints);
				LineDefinition++;
			}
			else { 
				OpenCurve = chai(OpenCurve); 
			}
		}
		else if (globals.CurveType == 2) {
			ClosedCurve = closed(ClosedCurve);
		}
		break;

		// decrease the LOD
	case '-':
		if (LineDefinition == 1) {
			OpenCurve = ControlPoints;
			LineDefinition = 1;
		}
		else if (LineDefinition > 1) {
			OpenCurve = unchai(OpenCurve);
			LineDefinition--;
		}
		break;

	default:
		break;
	}

	glutPostRedisplay();
}

void OnKeyPress1(unsigned char key, int, int) {
	
	switch (key) {

	case ' ':
		if (globals.ObjectType == 0)
			globals.ObjectType++;
		else if (globals.ObjectType == 1)
			globals.ObjectType = 0;
		break;

	case '+':
		if (globals.degree > 10 && globals.degree < 16)
			globals.degree -= 1;
		break;
	case '-':
		if (globals.degree > 9 && globals.degree < 15){
			globals.degree += 1;
		}
		break;

	default:
		break;
	}

	glutPostRedisplay();
}

//draw open curve
void chim(std::vector<Point> &A){
	glBegin(GL_LINE_STRIP);
	for (unsigned int i = 0; i < A.size(); i++) {
		Point ze = A[i];
		glVertex3f(ze.x(), ze.y(), ze.z());
	}

	glEnd();
}

//draw closed curve
void drawClosedCurve(std::vector<Point> &A){
	glBegin(GL_LINE_LOOP);
	for (unsigned int i = 0; i < A.size(); i++) {
		Point ze = A[i];
		glVertex3f(ze.x(), ze.y(), ze.z());
	}

	glEnd();
}

static void draw3DObject()
{
	//set our courseness
	int degree = 15;
	if (globals.degree != 0){
		degree = globals.degree;
	}
	else{ globals.degree = 15; }

	//Wire frame or Solid
	int WireF = globals.ObjectType;

	std::vector<Point> ori;
	if (globals.CurveType == 0){
		ori = OpenCurve;
	}
	else if (globals.CurveType == 1) {
		ori = drawBezierCurve(ControlPoints);
	}
	else if (globals.CurveType == 2) {
		ori = ClosedCurve;
	}
	else{
		exit(0);
	}

	//rotate the points around the x axis and put back in ori
	//double Rad = 0.261799388;	//15 degree rotation
	double Rad = degree *(M_PI / 180);
	int size = ori.size();
	int rot_start = 0;
	for (int i = rot_start; i < size; i++){
		Point xyz = ori[i];
		double rotx = xyz.x();
		double roty = (xyz.y()* cos(Rad)) - (xyz.z() * sin(Rad));
		double rotz = ((xyz.y() / (48 * 2))* sin(Rad)) + (xyz.z() * cos(Rad));
		Point final = Point(rotx, roty, rotz);
		ori.push_back(final);	//push the rotated value at end of vector
	}

	glTranslatef(5, 5, 1.5);	//Center is now 0,0,0

	for (int j = 0; j < (360.0 / degree); j++){
		glPushMatrix();
		glRotatef(degree *j, 1, 0, 0);
		//Print curve in 2d
		if (WireF == 0){
			//draw wire frame
			for (int i = 0; i < size - 1; i++){
				glBegin(GL_LINE_LOOP);
				glVertex3f(ori[i].x() / (64 * 2), ori[i].y() / (48 * 2), 0);
				glVertex3f(ori[i + size].x() / (64 * 2), ori[i + size].y() / (48 * 2), ori[i + size].z());
				glVertex3f(ori[i + size + 1].x() / (64 * 2), ori[i + size + 1].y() / (48 * 2), ori[i + size + 1].z());
				glColor3f(0, 1, 0);
				glVertex3f(ori[i + 1].x() / (64 * 2), ori[i + 1].y() / (48 * 2), 0);
				glEnd();
			}
		}
		else if (WireF == 1){
			//draw solid object
			for (int i = 0; i < size - 1; i++){
				glBegin(GL_QUADS);
				glColor3f(1, 0, 0);
				glVertex3f(ori[i].x() / (64 * 2), ori[i].y() / (48 * 2), 0);
				glColor3f(0, 0, 1);
				glVertex3f(ori[i + size].x() / (64 * 2), ori[i + size].y() / (48 * 2), ori[i + size].z());
				glColor3f(0, 1, 1);
				glVertex3f(ori[i + size + 1].x() / (64 * 2), ori[i + size + 1].y() / (48 * 2), ori[i + size + 1].z());
				glColor3f(1, 1, 1);
				glVertex3f(ori[i + 1].x() / (64 * 2), ori[i + 1].y() / (48 * 2), 0);
				glEnd();
			}
		}
		glPopMatrix();
	}
}

void myDisplay() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor3f(0, 1, 1);
	drawcontrol(ControlPoints);
	if (globals.CurveType == 0){
		chim(OpenCurve);
	}
	else if (globals.CurveType == 1){
		drawBezierCurve(ControlPoints);
	}
	else if (globals.CurveType == 2){
		drawClosedCurve(ClosedCurve);
	}
	glFlush();

	glutPostRedisplay();
}

void myDisplay1() {
	// clear the window with the predefined color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// setup viewing transformation
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	double r = globals.dist;
	double a = globals.alpha * M_PI / 180.0;
	double b = globals.beta * M_PI / 180.0;
	Point p(r * cos(a) * cos(b), r*sin(a)*cos(b), r*sin(b));
	Point c(5, 5, 1.5);
	p = p + c;
	gluLookAt(p.x(), p.y(), p.z(), c.x(), c.y(), c.z(), 0, 0, 1);
	GLfloat lightPos[] = { 10.0, 10.0, 10.0, 10.0 };
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

	// draw the scene
	draw3DObject();
	glFlush();

	glutPostRedisplay();
}


// when mouse button is clicked, we determine which viewing mode to
// initialize and also remember where the mouse was clicked
static void mouseClickCB(int button, int state, int x, int y)
{
	globals.mouseX = x;
	globals.mouseY = y;
	if (state == GLUT_UP) {
		globals.viewingMode = globals.NONE;
	}
	else if (button == GLUT_LEFT_BUTTON) {
		globals.viewingMode = globals.ROTATING;
	}
	else if (button == GLUT_MIDDLE_BUTTON) {
		globals.viewingMode = globals.ZOOMING;
	}
	else {
		globals.viewingMode = globals.NONE;
	}
}

// when user drags the mouse, we either rotate or zoom
static void mouseMotionCB(int x, int y)
{
	int dx = x - globals.mouseX;
	int dy = y - globals.mouseY;
	globals.mouseX = x;
	globals.mouseY = y;
	if (globals.viewingMode == globals.ROTATING) {
		globals.alpha -= dx / 10.0;
		globals.beta += dy / 10.0;
		if (globals.beta < -80) globals.beta = -80;
		if (globals.beta > 80) globals.beta = 80;
		glutPostRedisplay();
	}
	else if (globals.viewingMode == globals.ZOOMING) {
		globals.dist = std::max(1.0, globals.dist - dy / 10.0);
		glutPostRedisplay();
	}
}

//Recompute projection matrix and reset viewport
static void resizeCB(int w, int h)
{
	// setup perspective transformation
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, double(w) / h, 0.1, 100);
	glViewport(0, 0, w, h);
}

void myInit() {
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutInitWindowSize(640, 480);
	glutInitWindowPosition(100, 150);
	glutCreateWindow("2D Curve");

	glutKeyboardFunc(OnKeyPress);
	glutMouseFunc(myMouse);
	glutDisplayFunc(myDisplay);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glColor3f(1.0, 0.0, 0.0);
	glPointSize(10.0);
	glEnable(GL_POINT_SMOOTH);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, 640.0, 0.0, 480.0);

}

void myInit2() {


	globals.alpha = 30;
	globals.beta = 30;
	globals.dist = 10;
	globals.viewingMode = globals.NONE;
	globals.recursion = 3;
	globals.flat = true;

	// initial window size
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB
		| GLUT_MULTISAMPLE | GLUT_DEPTH);

	//glutInitWindowSize(640, 480);
	glutInitWindowSize(800, 680);
	glutInitWindowPosition(800, 150);

	// create a window
	glutCreateWindow("3D Curve");

	// register callbacks
	glutKeyboardFunc(OnKeyPress1);
	glutDisplayFunc(myDisplay1);
	glutReshapeFunc(resizeCB);
	glutMouseFunc(mouseClickCB);
	glutMotionFunc(mouseMotionCB);

	// use black as the background color
	glClearColor(0, 0, 0, 0);

	// enable depth buffer
	glEnable(GL_DEPTH_TEST);

	// this is for drawing transparencies
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// enable antialiasing (just in case)
	//    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_POLYGON_SMOOTH);

	// enable lighting
	glEnable(GL_LIGHTING);

	// enable use of glColor() to specify ambient & diffuse material properties
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	// set some common light & material properties
	glEnable(GL_LIGHT0);
	GLfloat ambientLight[] = { 0.1, 0.1, 0.1, 1.0 };
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
	GLfloat specularLight[] = { 1.0, 1.0, 1.0, 1.0 };
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
	GLfloat specularColor[] = { 0.7, 0.7, 0.7, 1.0 };
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularColor);
	GLfloat shininess[] = { 95.0 };
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

	//May use scaling, and possibly non-uniform
	glEnable(GL_NORMALIZE);
}

int main(int argc, char *argv[]) {
	glutInit(&argc, argv);
	myInit();
	myInit2();
	glutMainLoop();
	return 0;
}