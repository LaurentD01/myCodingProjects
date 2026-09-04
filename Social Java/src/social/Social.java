package social;

import java.util.*;

/**
 * Facade class for the social network system.
 * 
 */
public class Social {

  private final PersonRepository personRepository = new PersonRepository();
  private final GroupRepository groupRepository = new GroupRepository();
  private final PostRepository postRepository = new PostRepository();
  
  /**
   * Creates a new account for a person
   * 
   * @param code    nickname of the account
   * @param name    first name
   * @param surname last name
   * @throws PersonExistsException in case of duplicate code
   */
  public void addPerson(String code, String name, String surname) throws PersonExistsException {
    if (personRepository.findById(code).isPresent()){    // check if db already contains the code
        throw new PersonExistsException();
    }
    Person person = new Person(code, name, surname);    // create the person as a POJO
    personRepository.save(person);                      // save it to db
  }

  /**
   * Retrieves information about the person given their account code.
   * The info consists in name and surname of the person, in order, separated by
   * blanks.
   * 
   * @param code account code
   * @return the information of the person
   * @throws NoSuchCodeException if a person with that code does not exist
   */
  public String getPerson(String code) throws NoSuchCodeException {
    return personRepository.findById(code).map(Person::getInfo)
        .orElseThrow(NoSuchCodeException::new);
  }

  /**
   * Define a friendship relationship between two persons given their codes.
   * <p>
   * Friendship is bidirectional: if person A is adding as friend person B, that means
   * that person B automatically adds as friend person A.
   * 
   * @param codePerson1 first person code
   * @param codePerson2 second person code
   * @throws NoSuchCodeException in case either code does not exist
   */
  public void addFriendship(String codePerson1, String codePerson2)
      throws NoSuchCodeException {
        Person p1 = personRepository.findById(codePerson1).orElseThrow(NoSuchCodeException::new);
        Person p2 = personRepository.findById(codePerson2).orElseThrow(NoSuchCodeException::new);
        p1.getFriends().add(p2);
        p2.getFriends().add(p1);
        personRepository.update(p1);
        personRepository.update(p2);
  }

  /**
   * Retrieve the collection of their friends given the code of a person.
   *
   * @param codePerson code of the person
   * @return the list of person codes
   * @throws NoSuchCodeException in case the code does not exist
   */
  public Collection<String> listOfFriends(String codePerson)
      throws NoSuchCodeException {
    Person p = personRepository.findById(codePerson).orElseThrow(NoSuchCodeException::new);
    return p.getFriends().stream().map(Person::getCode).toList();
  }

  /**
   * Creates a new group with the given name
   * 
   * @param groupName name of the group
   * @throws GroupExistsException if a group with given name does not exist
   */
  public void addGroup(String groupName) throws GroupExistsException {
    if(groupRepository.findById(groupName).isPresent()){
      throw new GroupExistsException();
    }else{
      Group g = new Group(groupName);
      groupRepository.save(g);
    }
  }

  /**
   * Deletes the group with the given name
   * 
   * @param groupName name of the group
   * @throws NoSuchCodeException if a group with given name does not exist
   */
  public void deleteGroup(String groupName) throws NoSuchCodeException {
    groupRepository.delete(groupRepository.findById(groupName)
        .orElseThrow(NoSuchCodeException::new));
  }

  /**
   * Modifies the group name
   * 
   * @param groupName name of the group
   * @throws NoSuchCodeException if the original group name does not exist
   * @throws GroupExistsException if the target group name already exist
   */
  public void updateGroupName(String groupName, String newName) throws NoSuchCodeException, GroupExistsException {
    if(groupRepository.findById(newName).isPresent()){
      throw new GroupExistsException();
    }
    Group g = groupRepository.findById(groupName).orElseThrow(NoSuchCodeException::new);
    g.setName(newName);
    groupRepository.update(g);
  }

  /**
   * Retrieves the list of groups.
   * 
   * @return the collection of group names
   */
  public Collection<String> listOfGroups() {
    return groupRepository.findAll().stream().map(Group::getName).toList();
  }

  /**
   * Add a person to a group
   * 
   * @param codePerson person code
   * @param groupName  name of the group
   * @throws NoSuchCodeException in case the code or group name do not exist
   */
  public void addPersonToGroup(String codePerson, String groupName) throws NoSuchCodeException {
    Group g = groupRepository.findById(groupName).orElseThrow(NoSuchCodeException::new);
    Person p = personRepository.findById(codePerson).orElseThrow(NoSuchCodeException::new);
    g.addPerson(p);
    p.addGroup(g);
    groupRepository.update(g);
    personRepository.update(p);
  }

  /**
   * Retrieves the list of people on a group
   * 
   * @param groupName name of the group
   * @return collection of person codes
   */
  public Collection<String> listOfPeopleInGroup(String groupName) {
    return groupRepository.findById(groupName).orElseGet(Group::new).getMembers().stream()
        .map(Person::getCode).toList();
  }

  /**
   * Retrieves the code of the person having the largest
   * group of friends
   * 
   * @return the code of the person
   */
  public String personWithLargestNumberOfFriends() {
    return personRepository.findAll().stream()
        .max(Comparator.comparing(p -> p.getFriends().size()))
        .map(Person::getCode)
        .orElse("nessuno");
  }

  /**
   * Find the name of group with the largest number of members
   * 
   * @return the name of the group
   */
  public String largestGroup() {
    return groupRepository.findAll().stream()
        .max(Comparator.comparing(g -> g.getMembers().size()))
        .map(Group::getName)
        .orElse("nessuno");
  }

  /**
   * Find the code of the person that is member of
   * the largest number of groups
   * 
   * @return the code of the person
   */
  public String personInLargestNumberOfGroups() {
    return personRepository.findAll().stream()
        .max(Comparator.comparing(g -> g.getGroups().size()))
        .map(Person::getCode)
        .orElse("nessuno");
  }

  // R5

  /**
   * add a new post by a given account
   * 
   * @param authorCode the id of the post author
   * @param text   the content of the post
   * @return a unique id of the post
   */
  public String post(String authorCode, String text) throws NoSuchCodeException {
    Person p = personRepository.findById(authorCode).orElseThrow(NoSuchCodeException::new);
    Post post = new Post(authorCode, text);
    postRepository.save(post);
    p.addPost(post);
    personRepository.update(p);
    return post.getId();
  }

  /**
   * retrieves the content of the given post
   * 
   * @param pid    the id of the post
   * @return the content of the post
   */
  public String getPostContent(String pid) {
    return postRepository.findById(pid).get().getText();
  }

  /**
   * retrieves the timestamp of the given post
   * 
   * @param pid    the id of the post
   * @return the timestamp of the post
   */
  public long getTimestamp(String pid) {
    return postRepository.findById(pid).get().getTimestamp();
  }

  /**
   * returns the list of post of a given author paginated
   * 
   * @param author     author of the post
   * @param pageNo     page number (starting at 1)
   * @param pageLength page length
   * @return the list of posts id
   */
  public List<String> getPaginatedUserPosts(String author, int pageNo, int pageLength) {
    return postRepository.getPaginatedUserPosts(author, pageNo, pageLength).stream()
          .map(Post::getId).toList();
  }

  /**
   * returns the paginated list of post of friends.
   * The returned list contains the author and the id of a post separated by ":"
   * 
   * @param author     author of the post
   * @param pageNo     page number (starting at 1)
   * @param pageLength page length
   * @return the list of posts key elements
   */
  public List<String> getPaginatedFriendPosts(String author, int pageNo, int pageLength) {
    return postRepository.getPaginatedFriendsPosts(author, pageNo, pageLength).stream()
          .map(p -> p.getAuthorCode() + ":" + p.getId()).toList();
  }

}