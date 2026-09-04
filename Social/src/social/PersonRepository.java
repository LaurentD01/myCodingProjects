package social;

public class PersonRepository extends GenericRepository<Person, String> {

  public PersonRepository() {
    super(Person.class);
  }

}
