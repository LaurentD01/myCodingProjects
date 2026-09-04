package social;

import java.util.List;
import jakarta.persistence.TypedQuery;
import jakarta.persistence.EntityManager;

public class PostRepository extends GenericRepository<Post, String> {

  public PostRepository() {
    super(Post.class);
  }

  public List<Post> getPaginatedUserPosts(String authorCode, int pageNo, int pageLength) {
    EntityManager em = JPAUtil.getEntityManager();
    try {
      TypedQuery<Post> query = em.createQuery(
        "SELECT p FROM Post p " +
        "WHERE p.authorCode = :authorCode " +
        "ORDER BY p.timestamp DESC", Post.class
      );
      query.setParameter("authorCode", authorCode);
      query.setFirstResult((pageNo - 1) * pageLength);
      query.setMaxResults(pageLength);

      return query.getResultList();
    }
    finally {
      em.close();
    }
  }

  public List<Post> getPaginatedFriendsPosts(String authorCode, int pageNo, int pageLength) {
    EntityManager em = JPAUtil.getEntityManager();
    try {
      TypedQuery<Post> query = em.createQuery(
        "SELECT p FROM Post p " +
        "WHERE p.authorCode IN (SELECT f.code FROM Person a " +
        "JOIN a.friends f WHERE a.code = :authorCode) " +
        "ORDER BY p.timestamp DESC", Post.class
      );
      query.setParameter("authorCode", authorCode);
      query.setFirstResult((pageNo - 1) * pageLength);
      query.setMaxResults(pageLength);

      return query.getResultList();
    }
    finally {
      em.close();
    }
  }
    
}
