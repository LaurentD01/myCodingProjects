package social;

import jakarta.persistence.EntityManager;
import jakarta.persistence.EntityManagerFactory;
import jakarta.persistence.Persistence;

/**
 * Utility class to interact with the JPA factory classes.
 * <p>
 * This class is used by {@link GenericRepository} to create
 * and release the the {@link jakarta.persistence.EntityManager}
 * for the db.
 */
public class JPAUtil {

  private static final String TEST_PU_NAME = "socialPUTest";
  private static final String PU_NAME = "socialPU";

  private static EntityManagerFactory emf;
  private static String currentPUName = JPAUtil.PU_NAME;

  private JPAUtil(){ /* Cannot instantiate! */}

  private static EntityManagerFactory getCurrentFactory() {
    if (emf == null || !emf.isOpen()) {
      emf = Persistence.createEntityManagerFactory(currentPUName);
    }
    return emf;
  }

  /**
   * Enable test mode: the db is deleted on close.
   */
  public static void setTestMode() {
    currentPUName = JPAUtil.TEST_PU_NAME;
  }

  /**
   * Retrieves an entity manager to perform persistence operations.
   * 
   * @return entity manager object
   */
  public static EntityManager getEntityManager() {
    return getCurrentFactory().createEntityManager();
  }

  /**
   * Close the entity manager factory
   */
  public static void close() {
    if (emf!=null && emf.isOpen()) {
      emf.close();
    }
  }
}