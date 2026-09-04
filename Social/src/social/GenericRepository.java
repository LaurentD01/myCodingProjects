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
 * <p>
 * The recommended use involves:
 * <ul>
 * <li> define a new class that extends this one with the 
 *      appropriate type parameters: {@code E r}is the entity class
 *      and I is the id type, e.g.
 * 
 *      <pre>class PersonRepo extends GenericRepository<Person,String> { ... }</pre>
 * <li> define an attribute of the newly created repo type to represent the 
 *      collection of objects and to perform all operations
 * <li> use the provided methods to perform the CRUD operations:
 * <ul>
 *      <li> {@link #save(E)} to Create the record in the db corresponding to a given object
 *      <li> {@link #findById(I)} or {@link #findAll()} to Retrieve the info
 *      <li> {@link #update(E)} to Update thd db to reflect the object changes
 *      <li> {@link #delete(E)} to Delete the object
 * </ul>
 * <li> add additional methods in the specific repo class if you need to execute ad-hoc queries
 * </ul>
 * This class uses the {@link JPAUtil} class to create the {@link jakarta.persistence.EntityManager}
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