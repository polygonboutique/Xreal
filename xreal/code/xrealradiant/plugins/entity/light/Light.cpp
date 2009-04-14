#include "Light.h"

#include "iradiant.h"
#include "igrid.h"
#include "stringio.h"
#include "Doom3LightRadius.h"
#include "LightShader.h"
#include "../EntitySettings.h"

#include "LightNode.h"

namespace entity {

// Initialise the static default shader string
std::string LightShader::m_defaultShader = "";

// ------ Helper Functions ----------------------------------------------------------

/* greebo: Calculates the eight vertices defining the light corners as defined by the passed AABB.
 */
void light_vertices(const AABB& aabb_light, Vector3 points[6]) {
  Vector3 max(aabb_light.origin + aabb_light.extents);
  Vector3 min(aabb_light.origin - aabb_light.extents);
  Vector3 mid(aabb_light.origin);

  // top, bottom, tleft, tright, bright, bleft
  points[0] = Vector3(mid[0], mid[1], max[2]);
  points[1] = Vector3(mid[0], mid[1], min[2]);
  points[2] = Vector3(min[0], max[1], mid[2]);
  points[3] = Vector3(max[0], max[1], mid[2]);
  points[4] = Vector3(max[0], min[1], mid[2]);
  points[5] = Vector3(min[0], min[1], mid[2]);
}

/* greebo: light_draw() gets called by the render() function of the Light class.
 * It basically draws the small diamond representing the light origin
 */
void light_draw(const AABB& aabb_light, RenderStateFlags state) {
  Vector3 points[6];

  // Revert the light "diamond" to default extents for drawing
  AABB tempAABB;
  tempAABB.origin = aabb_light.origin;
  tempAABB.extents = Vector3(8,8,8);

   // Calculate the light vertices of this bounding box and store them into <points>
  light_vertices(tempAABB, points);

  	// greebo: Draw the small cube representing the light origin.
    typedef unsigned int index_t;
    const index_t indices[24] = {
      0, 2, 3,
      0, 3, 4,
      0, 4, 5,
      0, 5, 2,
      1, 2, 5,
      1, 5, 4,
      1, 4, 3,
      1, 3, 2
    };
    glVertexPointer(3, GL_DOUBLE, 0, points);
    glDrawElements(GL_TRIANGLES, sizeof(indices)/sizeof(index_t), RenderIndexTypeID, indices);
}

// ----- Light Class Implementation -------------------------------------------------

// Constructor
Light::Light(LightNode& node, const Callback& transformChanged, const Callback& boundsChanged, const Callback& evaluateTransform) :
	m_entity(node._entity),
	m_originKey(OriginChangedCaller(*this)),
	m_rotationKey(RotationChangedCaller(*this)),
	m_colour(Callback()),
	m_named(m_entity),
	m_radii_box(m_aabb_light.origin),
	_rCentre(m_doom3Radius.m_centerTransformed, m_aabb_light.origin, m_doom3Radius._centerColour),
	m_renderName(m_named, m_aabb_light.origin),
	m_useLightOrigin(false),
	m_useLightRotation(false),
	m_renderProjection(m_aabb_light.origin, m_doom3Projection, m_doom3Frustum),
	m_transformChanged(transformChanged),
	m_boundsChanged(boundsChanged),
	m_evaluateTransform(evaluateTransform)
{
	construct();
}

// Copy Constructor
Light::Light(const Light& other, LightNode& node, const Callback& transformChanged, const Callback& boundsChanged, const Callback& evaluateTransform) :
	m_entity(node._entity),
	m_originKey(OriginChangedCaller(*this)),
	m_rotationKey(RotationChangedCaller(*this)),
	m_colour(Callback()),
	m_named(m_entity),
	m_radii_box(m_aabb_light.origin),
	_rCentre(m_doom3Radius.m_centerTransformed, m_aabb_light.origin, m_doom3Radius._centerColour),
	m_renderName(m_named, m_aabb_light.origin),
	m_useLightOrigin(false),
	m_useLightRotation(false),
	m_renderProjection(m_aabb_light.origin, m_doom3Projection, m_doom3Frustum),
	m_transformChanged(transformChanged),
	m_boundsChanged(boundsChanged),
	m_evaluateTransform(evaluateTransform)
{
	construct();
}

/* greebo: This sets up the keyObservers so that the according classes get notified when any
 * of the key/values are changed.
 * Note, that the entity key/values are still empty at the point where this method is called.
 */
void Light::construct() {
	_colourLightTarget = Vector3(255,255,0);
	_colourLightUp = Vector3(255,0,255);
	_colourLightRight = Vector3(255,0,255);
	_colourLightStart = Vector3(0,0,0);
	_colourLightEnd = Vector3(0,0,0);

	default_rotation(m_rotation);
	m_aabb_light.origin = Vector3(0, 0, 0);
	default_extents(m_aabb_light.extents);

	m_keyObservers.insert("name", NamedEntity::IdentifierChangedCaller(m_named));
	m_keyObservers.insert("_color", Colour::ColourChangedCaller(m_colour));
	m_keyObservers.insert("origin", OriginKey::OriginChangedCaller(m_originKey));

	m_keyObservers.insert("angle", RotationKey::AngleChangedCaller(m_rotationKey));
	m_keyObservers.insert("rotation", RotationKey::RotationChangedCaller(m_rotationKey));
	m_keyObservers.insert("light_radius", Doom3LightRadius::LightRadiusChangedCaller(m_doom3Radius));
	m_keyObservers.insert("light_center", Doom3LightRadius::LightCenterChangedCaller(m_doom3Radius));
	m_keyObservers.insert("light_origin", Light::LightOriginChangedCaller(*this));
	m_keyObservers.insert("light_rotation", Light::LightRotationChangedCaller(*this));
	m_keyObservers.insert("light_fovX", Light::LightFovXChangedCaller(*this));
	m_keyObservers.insert("light_fovY", Light::LightFovYChangedCaller(*this));
	m_keyObservers.insert("light_near", Light::LightNearChangedCaller(*this));
	m_keyObservers.insert("light_far", Light::LightFarChangedCaller(*this));
	m_keyObservers.insert("texture", LightShader::ValueChangedCaller(m_shader));
	m_useLightFovX = m_useLightFovY = m_useLightNear = m_useLightFar = false;
	m_doom3ProjectionChanged = true;

	// set the colours to their default values
	m_doom3Radius.setCenterColour(m_entity.getEntityClass()->getColour());

	m_entity.setIsContainer(true);

	// Load the light colour (might be inherited)
	m_colour.colourChanged(m_entity.getKeyValue("_color"));
	m_shader.valueChanged(m_entity.getKeyValue("texture"));
}

void Light::updateOrigin() {
	m_boundsChanged();

	m_doom3Radius.m_changed();

    // Update the projection as well, if necessary
    if (isProjected())
        projectionChanged();

	GlobalSelectionSystem().pivotChanged();
}

void Light::originChanged() {
	m_aabb_light.origin = m_useLightOrigin ? m_lightOrigin : m_originKey.m_origin;
	updateOrigin();
}

void Light::lightOriginChanged(const std::string& value) {
	m_useLightOrigin = (!value.empty());
	if (m_useLightOrigin) {
		read_origin(m_lightOrigin, value);
	}
	originChanged();
}

void Light::lightFovXChanged(const std::string& value) {
	m_useLightFovX = (!value.empty());
	if (m_useLightFovX) {
		if(!string_parse_float(value.c_str(), m_lightFovX) || m_lightFovX == 0)
		{
			m_lightFovX = 45;
		}
	}
	projectionChanged();
}

void Light::lightFovYChanged(const std::string& value) {
	m_useLightFovY = (!value.empty());
	if (m_useLightFovY) {
		if(!string_parse_float(value.c_str(), m_lightFovY) || m_lightFovY == 0)
		{
			m_lightFovY = 45;
		}
	}
	projectionChanged();
}

void Light::lightNearChanged(const std::string& value) {
	m_useLightNear = (!value.empty());
	if (m_useLightNear) {
		if(!string_parse_float(value.c_str(), m_lightNear) || m_lightNear == 0)
		{
			m_lightNear = 1;
		}
	}
	projectionChanged();
}

void Light::lightFarChanged(const std::string& value) {
	m_useLightFar = (!value.empty());
	if (m_useLightFar) {
		if(!string_parse_float(value.c_str(), m_lightFar) || m_lightFar == 0)
		{
			m_lightFar = 300;
		}
	}
	projectionChanged();
}

void Light::writeLightOrigin() {
	write_origin(m_lightOrigin, &m_entity, "light_origin");
}

void Light::rotationChanged() {
	rotation_assign(m_rotation, m_useLightRotation ? m_lightRotation : m_rotationKey.m_rotation);
	GlobalSelectionSystem().pivotChanged();
}

void Light::lightRotationChanged(const std::string& value) {
	m_useLightRotation = (!value.empty());
	if(m_useLightRotation) {
		read_rotation(m_lightRotation, value);
	}
	rotationChanged();
}

/* greebo: Calculates the corners of the light radii box and rotates them according the rotation matrix.
 */
void Light::updateLightRadiiBox() const {
	// Get the rotation matrix
	const Matrix4& rotation = rotation_toMatrix(m_rotation);

	// Calculate the corners of the light radius box and store them into <m_radii_box.m_points>
	// For the first calculation an AABB with origin 0,0,0 is needed, the vectors get added
	// to the origin AFTER they are transformed by the rotation matrix
	aabb_corners(AABB(Vector3(0, 0, 0), m_doom3Radius.m_radiusTransformed), m_radii_box.m_points);

	// Transform each point with the given rotation matrix and add the vectors to the light origin
	matrix4_transform_point(rotation, m_radii_box.m_points[0]);
	m_radii_box.m_points[0] += m_aabb_light.origin;
	matrix4_transform_point(rotation, m_radii_box.m_points[1]);
	m_radii_box.m_points[1] += m_aabb_light.origin;
	matrix4_transform_point(rotation, m_radii_box.m_points[2]);
	m_radii_box.m_points[2] += m_aabb_light.origin;
	matrix4_transform_point(rotation, m_radii_box.m_points[3]);
	m_radii_box.m_points[3] += m_aabb_light.origin;
	matrix4_transform_point(rotation, m_radii_box.m_points[4]);
	m_radii_box.m_points[4] += m_aabb_light.origin;
	matrix4_transform_point(rotation, m_radii_box.m_points[5]);
	m_radii_box.m_points[5] += m_aabb_light.origin;
	matrix4_transform_point(rotation, m_radii_box.m_points[6]);
	m_radii_box.m_points[6] += m_aabb_light.origin;
	matrix4_transform_point(rotation, m_radii_box.m_points[7]);
	m_radii_box.m_points[7] += m_aabb_light.origin;
}

void Light::instanceAttach(const scene::Path& path) {
	if(++m_instanceCounter.m_count == 1) {
		m_entity.instanceAttach(path_find_mapfile(path.begin(), path.end()));
		m_entity.attach(m_keyObservers);
	}
}

void Light::instanceDetach(const scene::Path& path) {
	if(--m_instanceCounter.m_count == 0) {
		m_entity.detach(m_keyObservers);
		m_entity.instanceDetach(path_find_mapfile(path.begin(), path.end()));
	}
}

/* greebo: Snaps the current light origin to the grid.
 *
 * Note: This gets called when the light as a whole is selected, NOT in vertex editing mode
 */
void Light::snapto(float snap) {
	if (m_useLightOrigin) {
		m_lightOrigin = origin_snapped(m_lightOrigin, snap);
		writeLightOrigin();
	}
	else {
		m_originKey.m_origin = origin_snapped(m_originKey.m_origin, snap);
		m_originKey.write(&m_entity);
	}
}

void Light::setLightRadius(const AABB& aabb) {
	m_aabb_light.origin = aabb.origin;
	m_doom3Radius.m_radiusTransformed = aabb.extents;
}

void Light::transformLightRadius(const Matrix4& transform) {
	matrix4_transform_point(transform, m_aabb_light.origin);
}

void Light::revertTransform() {
	m_aabb_light.origin = m_useLightOrigin ? m_lightOrigin : m_originKey.m_origin;
	rotation_assign(m_rotation, m_useLightRotation ? m_lightRotation : m_rotationKey.m_rotation);
	m_doom3Radius.m_radiusTransformed = m_doom3Radius.m_radius;
	m_doom3Radius.m_centerTransformed = m_doom3Radius.m_center;
}

void Light::freezeTransform() {
	if (m_useLightOrigin) {
		m_lightOrigin = m_aabb_light.origin;
		writeLightOrigin();
	}
	else {
		m_originKey.m_origin = m_aabb_light.origin;
		m_originKey.write(&m_entity);
	}

    if (isProjected())
    {
#if 0
	    if (m_useLightTarget) {
			_lightTarget = _lightTargetTransformed;
			m_entity.setKeyValue("light_target", _lightTarget);
		}

		if (m_useLightUp) {
			_lightUp = _lightUpTransformed;
			m_entity.setKeyValue("light_up", _lightUp);
		}

		if (m_useLightRight) {
			_lightRight = _lightRightTransformed;
			m_entity.setKeyValue("light_right", _lightRight);
		}

		// Check the start and end (if the end is "above" the start, for example)
		checkStartEnd();

		if (m_useLightStart) {
			_lightStart = _lightStartTransformed;
			m_entity.setKeyValue("light_start", _lightStart);
		}

		if (m_useLightEnd) {
			_lightEnd = _lightEndTransformed;
			m_entity.setKeyValue("light_end", _lightEnd);
		}
#else
		if(m_useLightFovX) {
			m_entity.setKeyValue("light_fovX", floatToStr(m_lightFovX, "45"));
		}

		if(m_useLightFovY) {
			m_entity.setKeyValue("light_fovY", floatToStr(m_lightFovY, "45"));
		}

		if(m_useLightNear) {
			m_entity.setKeyValue("light_near", floatToStr(m_lightNear, "1"));
		}

		if(m_useLightFar) {
			m_entity.setKeyValue("light_far", floatToStr(m_lightFar, "300"));
		}
#endif
    }
    else
    {
    	// Save the light center to the entity key/values
		m_doom3Radius.m_center = m_doom3Radius.m_centerTransformed;
		m_entity.setKeyValue("light_center", m_doom3Radius.m_center);
    }

	if(m_useLightRotation) {
		rotation_assign(m_lightRotation, m_rotation);
		write_rotation(m_lightRotation, &m_entity, "light_rotation");
	}

	rotation_assign(m_rotationKey.m_rotation, m_rotation);
	write_rotation(m_rotationKey.m_rotation, &m_entity);

	if (!isProjected()) {
		m_doom3Radius.m_radius = m_doom3Radius.m_radiusTransformed;
		write_origin(m_doom3Radius.m_radius, &m_entity, "light_radius");
	}
}

entity::Doom3Entity& Light::getEntity() {
	return m_entity;
}
const entity::Doom3Entity& Light::getEntity() const {
	return m_entity;
}

/*Namespaced& Light::getNamespaced() {
	return m_nameKeys;
}*/
const NamedEntity& Light::getNameable() const {
	return m_named;
}
NamedEntity& Light::getNameable() {
	return m_named;
}
TransformNode& Light::getTransformNode() {
	return m_transform;
}
const TransformNode& Light::getTransformNode() const {
	return m_transform;
}

// Backend render function (GL calls)
void Light::render(const RenderInfo& info) const {
	light_draw(m_aabb_light, info.getFlags());
}

VolumeIntersectionValue Light::intersectVolume(const VolumeTest& volume, const Matrix4& localToWorld) const {
	return volume.TestAABB(m_aabb_light, localToWorld);
}

Doom3LightRadius& Light::getDoom3Radius() {
	return m_doom3Radius;
}

// Adds the light centre renderable to the given collector
void Light::renderLightCentre(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const {
	collector.Highlight(RenderableCollector::ePrimitive, false);
	collector.Highlight(RenderableCollector::eFace, false);
	collector.SetState(_rCentre.getShader(), RenderableCollector::eFullMaterials);
	collector.SetState(_rCentre.getShader(), RenderableCollector::eWireframeOnly);

	collector.addRenderable(_rCentre, localToWorld);
}

void Light::renderWireframe(RenderableCollector& collector,
							const VolumeTest& volume,
							const Matrix4& localToWorld,
							bool selected) const
{
	// Main render, submit the diamond that represents the light entity
	collector.SetState(
		m_entity.getEntityClass()->getWireShader(), RenderableCollector::eWireframeOnly
	);
	collector.SetState(
		m_entity.getEntityClass()->getWireShader(), RenderableCollector::eFullMaterials
	);
	collector.addRenderable(*this, localToWorld);

	// Render bounding box if selected or the showAllLighRadii flag is set
	if (selected || EntitySettings::InstancePtr()->showAllLightRadii()) {

		if (isProjected()) {
			// greebo: This is not much of an performance impact as the projection gets only recalculated when it has actually changed.
			projection();
#if 1
			// Tr3B:
			m_projectionOrientation = rotation();

			// HACK
			Matrix4 radiant2opengl(
			0,-1, 0, 0,
			0, 0, 1, 0,
			-1, 0, 0, 0,
			0, 0, 0, 1);
			m_projectionOrientation.multiplyBy(radiant2opengl);

			m_projectionOrientation.t().getVector3() = m_aabb_light.origin; //localAABB().origin;
			collector.addRenderable(m_renderProjection, m_projectionOrientation);
#else
			collector.addRenderable(m_renderProjection, localToWorld);
#endif
		}
		else {
			updateLightRadiiBox();
			collector.addRenderable(m_radii_box, localToWorld);
		}
	}

	// Render the name
	if (EntitySettings::InstancePtr()->renderEntityNames()) {
		collector.addRenderable(m_renderName, localToWorld);
	}
}

void Light::testSelect(Selector& selector, SelectionTest& test, const Matrix4& localToWorld) {
	test.BeginMesh(localToWorld);

	SelectionIntersection best;
	aabb_testselect(m_aabb_light, test, best);
	if (best.valid()) {
		selector.addIntersection(best);
	}
}

void Light::translate(const Vector3& translation) {
	m_aabb_light.origin = origin_translated(m_aabb_light.origin, translation);
}

void Light::rotate(const Quaternion& rotation) {
	rotation_rotate(m_rotation, rotation);
}

void Light::transformChanged() {
	revertTransform();
	m_evaluateTransform();
	updateOrigin();
}

const Matrix4& Light::getLocalPivot() const {
	m_localPivot = rotation_toMatrix(m_rotation);
	m_localPivot.t().getVector3() = m_aabb_light.origin;
	return m_localPivot;
}

void Light::setLightChangedCallback(const Callback& callback) {
	m_doom3Radius.m_changed = callback;
}

// greebo: This returns the AABB of the WHOLE light (this includes the volume and all its selectable vertices)
// Used to test the light for selection on mouse click.
const AABB& Light::aabb() const {
	if (isProjected()) {
#if 0
		float           xMin, xMax, yMin, yMax;
		float           zNear, zFar;

		zNear = m_lightNear;
		zFar = m_lightFar;

		xMax = zNear * tan(m_lightFovX * c_pi / 360.0f);
		xMin = -xMax;

		yMax = zNear * tan(m_lightFovY * c_pi / 360.0f);
		yMin = -yMax;

		// start with an empty AABB and include all the projection vertices
		Vector3 extends = Vector3(-zNear, xMin * zFar, yMin * zFar) - Vector3(zFar, xMax * zFar, yMax * zFar);
		m_doom3AABB = AABB(m_aabb_light.origin, extends);
#else
		const Matrix4& proj = projection();

		Matrix4 unproject(matrix4_full_inverse(proj));
		Vector3 points[8];
		aabb_corners(AABB(Vector3(0.5f, 0.5f, 0.5f), Vector3(0.5f, 0.5f, 0.5f)), points);
		points[0] = matrix4_transformed_vector4(unproject, Vector4(points[0], 1)).getProjected();
		points[1] = matrix4_transformed_vector4(unproject, Vector4(points[1], 1)).getProjected();
		points[2] = matrix4_transformed_vector4(unproject, Vector4(points[2], 1)).getProjected();
		points[3] = matrix4_transformed_vector4(unproject, Vector4(points[3], 1)).getProjected();
		points[4] = matrix4_transformed_vector4(unproject, Vector4(points[4], 1)).getProjected();
		points[5] = matrix4_transformed_vector4(unproject, Vector4(points[5], 1)).getProjected();
		points[6] = matrix4_transformed_vector4(unproject, Vector4(points[6], 1)).getProjected();
		points[7] = matrix4_transformed_vector4(unproject, Vector4(points[7], 1)).getProjected();

		m_doom3AABB = AABB(m_aabb_light.origin, Vector3());
		for(int i = 0; i < 8; i++) {
			points[i] += m_aabb_light.origin;
			m_doom3AABB.includePoint(points[i]);
		}
#endif
	}
	else {
		m_doom3AABB = AABB(m_aabb_light.origin, m_doom3Radius.m_radiusTransformed);

		// greebo: Make sure the light center (that maybe outside of the light volume) is selectable
		m_doom3AABB.includePoint(m_aabb_light.origin + m_doom3Radius.m_centerTransformed);
	}
	return m_doom3AABB;
}

const AABB& Light::localAABB() const {
  	return aabb();
}

/* This is needed for the drag manipulator to check the aabb of the light volume only (excl. the light center)
 */
const AABB& Light::lightAABB() const {
	_lightAABB = AABB(m_aabb_light.origin, m_doom3Radius.m_radiusTransformed);
  	return _lightAABB;
}

bool Light::testAABB(const AABB& other) const {
	if (isProjected()) {
		Matrix4 transform = rotation();
		transform.t().getVector3() = localAABB().origin;
		projection();
		Frustum frustum(frustum_transformed(m_doom3Frustum, transform));
		return frustum_test_aabb(frustum, other) != c_volumeOutside;
    }

	// test against an AABB which contains the rotated bounds of this light.
	const AABB& bounds = aabb();
	return aabb_intersects_aabb(other, AABB(
		bounds.origin,
		Vector3(
			static_cast<float>(fabs(m_rotation[0] * bounds.extents[0])
								+ fabs(m_rotation[3] * bounds.extents[1])
								+ fabs(m_rotation[6] * bounds.extents[2])),
			static_cast<float>(fabs(m_rotation[1] * bounds.extents[0])
								+ fabs(m_rotation[4] * bounds.extents[1])
								+ fabs(m_rotation[7] * bounds.extents[2])),
			static_cast<float>(fabs(m_rotation[2] * bounds.extents[0])
								+ fabs(m_rotation[5] * bounds.extents[1])
								+ fabs(m_rotation[8] * bounds.extents[2]))
		)
	));
}

const Matrix4& Light::rotation() const {
	m_doom3Rotation = rotation_toMatrix(m_rotation);
	return m_doom3Rotation;
}

/* greebo: This is needed by the renderer to determine the center of the light. It returns
 * the centerTransformed variable as the lighting should be updated as soon as the light center
 * is dragged.
 *
 * Note: In order to render projected lights correctly, I made the projection render code to use
 * this method to determine the center of projection, hence the if (isProjected()) clause
 */
const Vector3& Light::offset() const {
	if (isProjected())
    {
		return _projectionCenter;
	}
	else
    {
		return m_doom3Radius.m_centerTransformed;
	}
}
const Vector3& Light::colour() const {
	return m_colour.m_colour;
}

/* greebo: A light is projected, if the entity keys light_fovX, light_fovY, light_near and light_far are not empty.
 */
bool Light::isProjected() const {
	return m_useLightFovX && m_useLightFovY && m_useLightNear && m_useLightFar;
}

void Light::projectionChanged()
{
	m_doom3ProjectionChanged = true;
	m_doom3Radius.m_changed();

    // Calculate the projection centre
	_projectionCenter = m_aabb_light.origin;	// Tr3B: FIXME

	SceneChangeNotify();
}

const Matrix4& Light::projection() const {
	if (!m_doom3ProjectionChanged) {
		return m_doom3Projection;
	}

	m_doom3ProjectionChanged = false;
	m_doom3Projection = Matrix4::getIdentity();
	m_doom3Projection.translateBy(Vector3(0.5f, 0.5f, 0));
	m_doom3Projection.scaleBy(Vector3(0.5f, 0.5f, 1));


#if 1
    float           xMin, xMax, yMin, yMax;
    float           zNear, zFar;

    zNear = m_lightNear;
    zFar = m_lightFar;

    xMax = zNear * tan(m_lightFovX * c_pi / 360.0f);
    //xMax = zNear * tan(_lightRightTransformed.angle(_lightTargetTransformed));
    //xMax = zNear * tan(_lightTarget.angle(_lightRight));
    //xMax = zNear * tan(_lightRight.getLength() * M_PI / 360.0f);
    xMin = -xMax;

    yMax = zNear * tan(m_lightFovY * c_pi / 360.0f);
    //yMax = zNear * tan((_lightUpTransformed.getLength() * M_PI / 360.0f) * 0.25f);
    //yMax = zNear * tan(_lightTarget.angle(_lightUp));
    //xMax = zNear * tan(_lightUp.getLength() * M_PI / 360.0f);
    yMin = -yMax;

    Matrix4 frustum = matrix4_frustum(xMin, xMax, yMin, yMax, zNear, zFar);
    m_doom3Projection.multiplyBy(frustum);
    m_doom3Frustum = frustum_from_viewproj(m_doom3Projection);

#else
	Plane3 lightProject[4];

	// If there is a light_start key set, use this, otherwise use the unit vector of the target direction
	Vector3 start = m_useLightStart && m_useLightEnd ? _lightStartTransformed : _lightTargetTransformed.getNormalised();

	// If there is no light_end, but a light_start, assume light_end = light_target
	Vector3 stop = m_useLightStart && m_useLightEnd ? _lightEndTransformed : _lightTargetTransformed;

	double rLen = _lightRightTransformed.getLength();
	Vector3 right = _lightRightTransformed / rLen;
	double uLen = _lightUpTransformed.getLength();
	Vector3 up = _lightUpTransformed / uLen;
	Vector3 normal = up.crossProduct(right).getNormalised();

	double dist = _lightTargetTransformed.dot(normal);
	if ( dist < 0 ) {
		dist = -dist;
		normal = -normal;
	}

	right *= ( 0.5f * dist ) / rLen;
	up *= -( 0.5f * dist ) / uLen;

	lightProject[2] = Plane3(normal, 0);
	lightProject[0] = Plane3(right, 0);
	lightProject[1] = Plane3(up, 0);

	// now offset to center
	Vector4 targetGlobal(_lightTargetTransformed, 1);
    {
		double a = targetGlobal.dot(plane3_to_vector4(lightProject[0]));
		double b = targetGlobal.dot(plane3_to_vector4(lightProject[2]));
		double ofs = 0.5 - a / b;
		plane3_to_vector4(lightProject[0]) += plane3_to_vector4(lightProject[2]) * ofs;
	}
	{
		double a = targetGlobal.dot(plane3_to_vector4(lightProject[1]));
		double b = targetGlobal.dot(plane3_to_vector4(lightProject[2]));
		double ofs = 0.5 - a / b;
		plane3_to_vector4(lightProject[1]) += plane3_to_vector4(lightProject[2]) * ofs;
	}

	// set the falloff vector
	Vector3 falloff = stop - start;
	double length = falloff.getLength();
	falloff /= length;
	if ( length <= 0 ) {
		length = 1;
	}
	falloff *= (1.0f / length);
	lightProject[3] = Plane3(falloff, -start.dot(falloff));

	// we want the planes of s=0, s=q, t=0, and t=q
	m_doom3Frustum.left = lightProject[0];
	m_doom3Frustum.bottom = lightProject[1];
	m_doom3Frustum.right = Plane3(lightProject[2].normal() - lightProject[0].normal(), lightProject[2].dist() - lightProject[0].dist());
	m_doom3Frustum.top = Plane3(lightProject[2].normal() - lightProject[1].normal(), lightProject[2].dist() - lightProject[1].dist());

	// we want the planes of s=0 and s=1 for front and rear clipping planes
	m_doom3Frustum.front = lightProject[3];

	m_doom3Frustum.back = lightProject[3];
	m_doom3Frustum.back.dist() -= 1.0f;
	m_doom3Frustum.back = -m_doom3Frustum.back;

	Matrix4 test(matrix4_from_planes(m_doom3Frustum.left, m_doom3Frustum.right, m_doom3Frustum.bottom, m_doom3Frustum.top, m_doom3Frustum.front, m_doom3Frustum.back));
	m_doom3Projection.multiplyBy(test);

	m_doom3Frustum.left = m_doom3Frustum.left.getNormalised();
	m_doom3Frustum.right = m_doom3Frustum.right.getNormalised();
	m_doom3Frustum.bottom = m_doom3Frustum.bottom.getNormalised();
	m_doom3Frustum.top = m_doom3Frustum.top.getNormalised();
	m_doom3Frustum.back = m_doom3Frustum.back.getNormalised();
	m_doom3Frustum.front = m_doom3Frustum.front.getNormalised();
	//m_doom3Projection.scaleBy(Vector3(1.0 / 128, 1.0 / 128, 1.0 / 128));
#endif

	return m_doom3Projection;
}

ShaderPtr Light::getShader() const {
	return m_shader.get();
}

} // namespace entity
