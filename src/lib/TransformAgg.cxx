/***********************************************************************
 * The rDock program was developed from 1998 - 2006 by the software team
 * at RiboTargets (subsequently Vernalis (R&D) Ltd).
 * In 2006, the software was licensed to the University of York for
 * maintenance and distribution.
 * In 2012, Vernalis and the University of York agreed to release the
 * program as Open Source software.
 * This version is licensed under GNU-LGPL version 3.0 with support from
 * the University of Barcelona.
 * http://rdock.sourceforge.net/
 ***********************************************************************/

#include "TransformAgg.h"

using namespace rxdock;

// Static data member for class type
std::string TransformAgg::_CT("TransformAgg");

////////////////////////////////////////
// Constructors/destructors
TransformAgg::TransformAgg(const std::string &strName)
    : BaseTransform(_CT, strName) {
#ifdef _DEBUG
  std::cout << _CT << " parameterised constructor" << std::endl;
#endif //_DEBUG
  _RBTOBJECTCOUNTER_CONSTR_(_CT);
}

TransformAgg::~TransformAgg() {
  // Delete all our children
#ifdef _DEBUG
  std::cout << _CT << "::~" << _CT << "(): Deleting child transforms of "
            << GetName() << std::endl;
#endif //_DEBUG
       // We need to iterate using a while loop because each deletion will
       // reduce the size of m_sf, hence conventional iterators would become
       // invalid
  while (m_transforms.size() > 0) {
    BaseTransform *pTransform = m_transforms.back();
    // Assertion: parent of child is this object
    Assert<Assertion>(!TRANSFORMAGG_CHECK || pTransform->m_parent == this);
#ifdef _DEBUG
    std::cout << "Deleting " << pTransform->GetName() << " from " << GetName()
              << std::endl;
#endif //_DEBUG
    delete pTransform;
  }
  _RBTOBJECTCOUNTER_DESTR_(_CT);
}

////////////////////////////////////////
// Public methods
////////////////

// Aggregate handling methods
void TransformAgg::Add(BaseTransform *pTransform) {
  // By first orphaning the transform to be added,
  // we handle attempts to readd existing children automatically,
  pTransform->Orphan();
  pTransform->m_parent = this;
#ifdef _DEBUG
  std::cout << _CT << "::Add(): Adding " << pTransform->GetName() << " to "
            << GetName() << std::endl;
#endif //_DEBUG
  m_transforms.push_back(pTransform);
}

void TransformAgg::Remove(BaseTransform *pTransform) {
  BaseTransformListIter iter =
      std::find(m_transforms.begin(), m_transforms.end(), pTransform);
  if (iter == m_transforms.end()) {
    throw BadArgument(_WHERE_,
                      "Remove(): pTransform is not a member of this aggregate");
  } else {
    // Assertion: parent of child is this object
    Assert<Assertion>(!TRANSFORMAGG_CHECK || pTransform->m_parent == this);
#ifdef _DEBUG
    std::cout << _CT << "::Remove(): Removing " << pTransform->GetName()
              << " from " << GetName() << std::endl;
#endif //_DEBUG
    m_transforms.erase(iter);
    pTransform->m_parent = nullptr; // Nullify the parent pointer of the child
                                    // that has been removed
  }
}

bool TransformAgg::isAgg() const { return true; }
unsigned int TransformAgg::GetNumTransforms() const {
  return m_transforms.size();
}
BaseTransform *TransformAgg::GetTransform(unsigned int iTransform) const {
  if (iTransform >= m_transforms.size()) {
    throw BadArgument(_WHERE_, "GetTransform(): iTransform out of range");
  } else {
    return m_transforms[iTransform];
  }
}

// WorkSpace handling methods
// Register scoring function with a workspace
// Aggregate version registers all children, but NOT itself
//(Aggregates are just containers, and have no need for model information
void TransformAgg::Register(WorkSpace *pWorkSpace) {
#ifdef _DEBUG
  std::cout << _CT << "::Register(): Registering child transforms of "
            << GetName() << std::endl;
#endif //_DEBUG
  for (BaseTransformListIter iter = m_transforms.begin();
       iter != m_transforms.end(); iter++) {
    (*iter)->Register(pWorkSpace);
  }
}

// Unregister with a workspace
// Aggregate version unregisters all children, but NOT itself
void TransformAgg::Unregister() {
#ifdef _DEBUG
  std::cout << _CT << "::Unregister(): Unregistering child transforms of "
            << GetName() << std::endl;
#endif //_DEBUG
  for (BaseTransformListIter iter = m_transforms.begin();
       iter != m_transforms.end(); iter++) {
    (*iter)->Unregister();
  }
}

// Override Observer pure virtual
// Notify observer that subject has changed
// Does nothing in TransformAgg as aggregates do not require updating
void TransformAgg::Update(Subject *theChangedSubject) {
  //	if (theChangedSubject == GetWorkSpace()) {
  //#ifdef _DEBUG
  //         std::cout << _CT << "::Update(): " << GetName() << " received
  //         update
  // from workspace" << std::endl; #endif //_DEBUG
  //	}
}

// Request Handling method
// Aggregates handle the request themselves first, then cascade to all children
void TransformAgg::HandleRequest(RequestPtr spRequest) {
  BaseObject::HandleRequest(spRequest);
  for (BaseTransformListIter iter = m_transforms.begin();
       iter != m_transforms.end(); iter++) {
    (*iter)->HandleRequest(spRequest);
  }
}

// Virtual function for dumping scoring function details to an output stream
// Called by operator <<
void TransformAgg::Print(std::ostream &s) const {
  // First print the parameters for this aggregate
  BaseObject::Print(s);
  // Now call Print() for each of the children
  for (BaseTransformListConstIter iter = m_transforms.begin();
       iter != m_transforms.end(); iter++) {
    (*iter)->Print(s);
  }
}

// Actually apply the transform
// Aggregate version loops over all child transforms
void TransformAgg::Execute() {
  for (BaseTransformListConstIter iter = m_transforms.begin();
       iter != m_transforms.end(); iter++) {
    (*iter)->Go();
  }
}