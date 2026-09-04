package social;

import java.util.List;
import java.util.Objects;
import java.util.Optional;

import jakarta.persistence.Entity;
import jakarta.persistence.EntityManager;
import jakarta.persistence.EntityTransaction;

/**
 * This class represent a generic repository, and can be used 
 * to implement the ORM repository pattern.
 * <br>
 * This class allows performing the CRUD (Create Retrieve Update and Delete)
 * operations for a specific entity through the JPA API.
 */
public class GenericRepository<E, I> {

  private final Class<E> entityClass;
  protected final String entityName;

  /**
   * The constructor that should be invoked by derived classes
   * require the class of the entity to perform all operations
   * 
   * @param entityClass the class of the entity
   */
  protected GenericRepository(Class<E> entityClass) {
    Objects.requireNonNull(entityClass);
    this.entityClass = entityClass;
    entityName = getEntityName(entityClass);
  }

  protected static String getEntityName(Class<?> entityClass){
    Entity ea = entityClass.getAnnotation(jakarta.persistence.Entity.class);
    if(ea==null) throw new IllegalArgumentException("Class " + entityClass.getName() + " must be annotated as @Entity");
    if(ea.name().isEmpty()) return entityClass.getSimpleName();
    return ea.name();
  } 

  /**
   * Return an {@code Optional<E>} containing the object corresponding to the given id
   * 
   * @param id the id of the required object
   * 
   * @return the optional object or an empty optional if not found
   */
  public Optional<E> findById(I id) {
    EntityManager em = JPAUtil.getEntityManager();
    E entity = em.find(entityClass, id);
    em.close();
    return Optional.ofNullable(entity);
  }

  /**
   * Retrieves all the objects corresponding to all rows of the entity
   * 
   * @return a list with all the entity instances
   */
  public List<E> findAll() {
    EntityManager em = JPAUtil.getEntityManager();
    List<E> result = em.createQuery("SELECT e FROM " + entityName + " e", entityClass)
        .getResultList();
    em.close();
    return result;
  }


  /**
   * Create a new row in the db corresponding to the given object.
   * <p>
   * Usually an object is created a POJO (Plain Old Java Object) and then
   * it must be explicitly persisted.
   * 
   * @param entity the entity to be persisted
   */
  public void save(E entity) {
    EntityManager em = JPAUtil.getEntityManager();
    EntityTransaction tx = em.getTransaction();
    try {
      tx.begin();
      em.persist(entity);
      tx.commit();
    } catch (RuntimeException ex) {
      if (tx.isActive())
        tx.rollback();
      throw ex;
    } finally {
      em.close();
    }
  }

  /**
   * Updates an object whose state has been modified.
   * <p>
   * It is important to remember that changes to objects
   * are not automatically persisted, the new state
   * must be merged into the db explicitly.
   * 
   * @param entity the object to be updated
   */
  public void update(E entity) {
    EntityManager em = JPAUtil.getEntityManager();
    EntityTransaction tx = em.getTransaction();
    try {
      tx.begin();
      em.merge(entity);
      tx.commit();
    } catch (RuntimeException ex) {
      if (tx.isActive())
        tx.rollback();
      throw ex;
    } finally {
      em.close();
    }
  }

  /**
   * Delete and object from the db
   * 
   * @param entity the object to be deleted
   */
  public void delete(E entity) {
    EntityManager em = JPAUtil.getEntityManager();
    EntityTransaction tx = em.getTransaction();
    try {
      tx.begin();
      em.remove(em.contains(entity) ? entity : em.merge(entity));
      tx.commit();
    } catch (RuntimeException ex) {
      if (tx.isActive())
        tx.rollback();
      throw ex;
    } finally {
      em.close();
    }
  }
}
